#include "GitHubChecker.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

// Include httplib.h before Windows.h or include Windows.h by defining
// WIN32_LEAN_AND_MEAN beforehand.
#include <httplib.h>

#ifndef DZSIM_WEB_PORT
#include <Windows.h>
#include <shellapi.h>
#endif

#include <Tracy.hpp>

#include <Corrade/Utility/Debug.h>
#include <Magnum/Magnum.h>
#include <json.hpp>

#include "cmake_build_settings.h"

using namespace Magnum;
using json = nlohmann::json;


// ---- GLOBAL UPDATE CHECK STATE ----
static size_t g_update_checker_instance_count = 0;

static std::atomic<GitHubChecker::UpdateStatus> g_cur_update_state
    = GitHubChecker::UpdateStatus::NOT_CHECKED;

static std::mutex g_cur_motd_mutex; // Protects g_cur_motd
static std::string g_cur_motd = "";

// Performs async check
static std::thread g_check_thread;

// If the most recent async check has put its result in g_cur_update_state
static std::atomic<bool> g_is_async_check_finished = true;


// Notes on GitHub API usage:
//   - Make requests for a single user or client ID serially. Do not make
//     requests for a single user or client ID concurrently.
//   - When a new REST API version is released, the previous API version will be
//     supported for at least 24 more months following the release of the new
//     API version.
//     If you specify an API version that is no longer supported, you will
//     receive a 400 error.
//   - When making unauthorized requests too frequently, a rate limit is
//     imposed on the IP that's making these requests.

// If operation fails, returns a JSON null value
static json MakeGetJsonRequestFromGitHub(httplib::Client& gh_api_cli,
    const std::string& request_path)
{
    httplib::Headers headers = {
        {
            // Recommended 'accept' header
            "Accept: application/vnd.github+json",
            // Specifying targeted version of GitHub's REST API, here the latest
            // one at the time of writing (2022-12-16)
            "X-GitHub-Api-Version: 2022-11-28"
        }
    };

    auto res = gh_api_cli.Get(request_path.c_str(), headers);
    if (!res) {
        Debug{} << "[GitHubChecker]" << "ERROR: GET" << request_path.c_str()
            << "failed with error code:" << (int)res.error();
        return nullptr; // Return JSON null value
    }

    if (res->status != 200) { // HTTP response status code 200 means "Success"
        Debug{} << "[GitHubChecker]" << "ERROR: GET" << request_path.c_str()
            << "; status = " << res->status << "; body = ";
        Debug{} << "   " << res->body.c_str();
        return nullptr; // Return JSON null value
    }

    // While parsing JSON: Ignore comments and don't throw exceptions
    json obj = json::parse(res->body, nullptr, false, true);

    if (obj.is_discarded()) { // If parse error occurred
        Debug{} << "[GitHubChecker]" << "ERROR while parsing received JSON "
            "from: GET" << request_path.c_str() << "; input = ";
        Debug{} << "   " << res->body.c_str();
        return nullptr; // Return JSON null value
    }
    return obj;
}

// Returns how many unauthorized requests to GitHub's API are left.
// Returns -1 if any error occurs or if GitHub's response is invalid.
// Makes a GitHub API request, but is NOT subject to request rate limiting.
// See https://docs.github.com/en/rest/overview/resources-in-the-rest-api?apiVersion=2022-11-28#rate-limiting
// and https://docs.github.com/en/rest/rate-limit?apiVersion=2022-11-28
static int GetRemainingGitHubRequestCount(httplib::Client& gh_api_cli) {
    const json rate_limit_obj
        = MakeGetJsonRequestFromGitHub(gh_api_cli, "/rate_limit");

    if (rate_limit_obj.is_null()) // If request failed
        return -1;

    if (rate_limit_obj.contains("resources")) {
        auto& resources_obj = rate_limit_obj["resources"];

        if (resources_obj.contains("core")) {
            auto& core_obj = resources_obj["core"];

            if (core_obj.contains("remaining")) {
                auto& remaining_obj = core_obj["remaining"];

                if (remaining_obj.is_number_integer()) {
                    int cnt = remaining_obj.get<int>();
                    Debug{} << "[GitHubChecker] Remaining GitHub requests:" << cnt;

                    if (cnt < 0) // Invalid GitHub response
                        return -1;
                    return cnt;
                }
            }
        }
    }

    Debug{} << "[GitHubChecker] GetRemainingGitHubRequestCount: "
        "ERROR, could not find target value in received JSON:";
    Debug{} << rate_limit_obj.dump(2).c_str();
    return -1;
}

// Returns tag name of the newest GitHub release.
// Returns an empty string if any error occurs or if GitHub's response is invalid.
// Makes 1 GitHub API request, subject to request rate limiting.
// See https://docs.github.com/en/rest/releases/releases?apiVersion=2022-11-28#get-the-latest-release
static std::string GetLatestGitHubReleaseTag(httplib::Client& gh_api_cli) {
    const json latest_release_obj = MakeGetJsonRequestFromGitHub(gh_api_cli,
        "/repos/lacyyy/DZSimulator/releases/latest");

    if (latest_release_obj.is_null()) // If request failed
        return "";

    if (latest_release_obj.contains("tag_name")) {
        auto& tag_name_obj = latest_release_obj["tag_name"];

        if (tag_name_obj.is_string()) {
            std::string tag_name = tag_name_obj.get<std::string>();
            Debug{} << "[GitHubChecker] Tag name of latest release on GitHub: "
                "\"" << tag_name.c_str() << "\"";
            return tag_name;
        }
    }

    Debug{} << "[GitHubChecker] GetLatestGitHubReleaseTag: "
        "ERROR, could not find target value in received JSON:";
    Debug{} << latest_release_obj.dump(2).c_str();
    return "";
}

// Returns text content of DZSimulator's MOTD gist.
// Returns an empty string if any error occurs or if GitHub's response is invalid.
// Makes 1 GitHub API request, subject to request rate limiting.
// See https://docs.github.com/en/rest/gists/gists?apiVersion=2022-11-28#get-a-gist
static std::string GetMotdGistContent(httplib::Client& gh_api_cli) {
    const json gist_obj = MakeGetJsonRequestFromGitHub(gh_api_cli,
        "/gists/78dc2c304cfc9d0db7c1c6e9e2859fab");

    if (gist_obj.is_null()) // If request failed
        return "";

    if (gist_obj.contains("files")) {
        auto& files_obj = gist_obj["files"];

        if (files_obj.is_object()) {

            // Get content of first file we find
            for (auto& elem : files_obj.items()) {
                auto& file_obj = elem.value();

                if (file_obj.contains("content")) {
                    auto& content_obj = file_obj["content"];

                    if (content_obj.is_string()) {
                        std::string motd = content_obj.get<std::string>();
                        Debug{} << "[GitHubChecker] Current MOTD from GitHub: "
                            "\"" << motd.c_str() << "\"";
                        return motd;
                    }
                }
            }
        }
    }

    Debug{} << "[GitHubChecker] GetMotdGistContent: "
        "ERROR, could not find target value in received JSON:";
    Debug{} << gist_obj.dump(2).c_str();
    return "";
}

// Compares a given git tag with the version of the currently running DZSimulator.
// Returns  1 if the tag is from a newer version.
// Returns  0 if the tag is NOT from a newer version.
// Returns -1 if the tag has an invalid format
static int IsGitTagFromNewerVersion(const std::string& git_tag) {
    const size_t CURRENT_VER_MAJOR = DZ_SIM_VERSION_MAJOR;
    const size_t CURRENT_VER_MINOR = DZ_SIM_VERSION_MINOR;
    const size_t CURRENT_VER_PATCH = DZ_SIM_VERSION_PATCH;
    
    // Extract MAJOR, MINOR and PATCH numbers from the given tag name

    size_t ver_start_pos = 0;

    // Get pos of first non-space char
    while (ver_start_pos < git_tag.length()) {
        if (!std::isspace(git_tag[ver_start_pos]))
            break;
        ver_start_pos++;
    }

    if (ver_start_pos == git_tag.length()) // git tag only has space chars
        return -1;

    // Skip optional leading 'V' character
    if (git_tag[ver_start_pos] == 'V' || git_tag[ver_start_pos] == 'v')
        ver_start_pos++;

    std::string ver_str = git_tag.substr(ver_start_pos);
    if (ver_str.length() == 0)
        return -1;

    size_t pos = 0; // index into ver_str

    // Extract MAJOR value string
    size_t major_str_start_pos = pos;
    while (pos < ver_str.length()) {
        if (!std::isdigit(ver_str[pos]))
            break;
        pos++;
    }
    size_t major_str_len = pos - major_str_start_pos;
    if (major_str_len == 0 || major_str_len > 5)
        return -1;
    std::string git_tag_major_str =
        ver_str.substr(major_str_start_pos, major_str_len);
    int git_tag_major_num = std::stoi(git_tag_major_str);

    if (git_tag_major_num > CURRENT_VER_MAJOR) return 1;
    if (git_tag_major_num < CURRENT_VER_MAJOR) return 0;

    // Advance to next '.' char
    while (pos < ver_str.length()) {
        if (ver_str[pos] == '.')
            break;
        pos++;
    }
    pos++; // Advance past '.' char

    // Extract MINOR value string
    size_t minor_str_start_pos = pos;
    while (pos < ver_str.length()) {
        if (!std::isdigit(ver_str[pos]))
            break;
        pos++;
    }
    size_t minor_str_len = pos - minor_str_start_pos;
    if (minor_str_len == 0 || minor_str_len > 5)
        return -1;
    std::string git_tag_minor_str =
        ver_str.substr(minor_str_start_pos, minor_str_len);
    int git_tag_minor_num = std::stoi(git_tag_minor_str);

    if (git_tag_minor_num > CURRENT_VER_MINOR) return 1;
    if (git_tag_minor_num < CURRENT_VER_MINOR) return 0;

    // Advance to next '.' char
    while (pos < ver_str.length()) {
        if (ver_str[pos] == '.')
            break;
        pos++;
    }
    pos++; // Advance past '.' char

    // Extract PATCH value string
    size_t patch_str_start_pos = pos;
    while (pos < ver_str.length()) {
        if (!std::isdigit(ver_str[pos]))
            break;
        pos++;
    }
    size_t patch_str_len = pos - patch_str_start_pos;
    if (patch_str_len == 0 || patch_str_len > 5)
        return -1;
    std::string git_tag_patch_str =
        ver_str.substr(patch_str_start_pos, patch_str_len);
    int git_tag_patch_num = std::stoi(git_tag_patch_str);

    if (git_tag_patch_num > CURRENT_VER_PATCH) return 1;
    if (git_tag_patch_num < CURRENT_VER_PATCH) return 0;

    return 0; // major, minor and patch versions are identical at this point
}

GitHubChecker::GitHubChecker()
{
    g_update_checker_instance_count++;
    if (g_update_checker_instance_count > 1) {
        Debug{} << "[GitHubChecker] ERROR! CANNOT CONSTRUCT A SECOND INSTANCE "
            "OF GitHubChecker!";
        std::terminate();
    }
}

GitHubChecker::~GitHubChecker()
{
    g_update_checker_instance_count--;

    // What if HTTP library calls block forever? Bad luck I guess...
    if (g_check_thread.joinable()) {
        Debug{} << "[GitHubChecker] Joining thread...";
        g_check_thread.join();
        Debug{} << "[GitHubChecker] Joined!";
    }
}

void GitHubChecker::StartAsyncUpdateAndMotdCheck()
{
    ZoneScoped;
#ifdef DZSIM_WEB_PORT
    return;
#else
    if (!g_is_async_check_finished)
        return;
    
    if (g_check_thread.joinable()) {
        Debug{} << "[GitHubChecker] Joining thread...";
        g_check_thread.join();
        Debug{} << "[GitHubChecker] Joined!";
    }

    // Start async check
    g_is_async_check_finished = false;
    g_check_thread = std::thread([] {
        tracy::SetThreadName("GitHubChecker Thread");

        // GitHub's API doesn't accept HTTP connections, only HTTPS
        httplib::Client cli("https://api.github.com");
        
        const size_t REQUIRED_REQUEST_COUNT = 2;
        int cnt = GetRemainingGitHubRequestCount(cli);

        // If the current rate limit window doesn't have enough requests left
        if (cnt >= 0 && cnt < REQUIRED_REQUEST_COUNT) {
            // Doing the needed requests would lead to exceeding the API's rate limit
            Debug{} << "[GitHubChecker] GitHub's API limit was reached! Will not "
                "check for updates. Can try again later.";
            // Having reached the limit indicates that we checked more than once
            // in the past hour. In this case we don't have to notify the user
            // of anything and can just act like nothing's happening.
            g_cur_update_state = GitHubChecker::UpdateStatus::NO_UPDATE_AVAILABLE;
            {
                std::lock_guard<std::mutex> lock(g_cur_motd_mutex);
                g_cur_motd = "";
            }
            g_is_async_check_finished = true;
            return;
        }

        // If we don't know how many requests are allowed, just try to make requests!
        if (cnt == -1) {} // Act like (cnt >= REQUIRED_REQUEST_COUNT)

        std::string new_motd = GetMotdGistContent(cli);
        {
            std::lock_guard<std::mutex> lock(g_cur_motd_mutex);
            g_cur_motd = std::move(new_motd);
        }

        std::string latest_release_tag = GetLatestGitHubReleaseTag(cli);

        // If latest version tag could not be retrieved
        if (latest_release_tag.empty()) {
            Debug{} << "[GitHubChecker] Unable to check for updates on GitHub!";
            g_cur_update_state = GitHubChecker::UpdateStatus::UPDATE_CHECK_FAILED;
            g_is_async_check_finished = true;
            return;
        }

        int ret = IsGitTagFromNewerVersion(latest_release_tag);

        if (ret == -1) {
            Debug{} << "[GitHubChecker] ERROR: Newest GitHub tag is invalid";
            g_cur_update_state = GitHubChecker::UpdateStatus::UPDATE_CHECK_FAILED;
            g_is_async_check_finished = true;
            return;
        }

        bool is_update_available = ret == 1;

        if (is_update_available)
            g_cur_update_state = GitHubChecker::UpdateStatus::UPDATE_AVAILABLE;
        else
            g_cur_update_state = GitHubChecker::UpdateStatus::NO_UPDATE_AVAILABLE;

        g_is_async_check_finished = true;
    });
#endif
}

bool GitHubChecker::IsAsyncUpdateAndMotdCheckFinished()
{
    return g_is_async_check_finished;
}

GitHubChecker::UpdateStatus GitHubChecker::GetUpdateStatus()
{
    return g_cur_update_state;
}

std::string GitHubChecker::GetMotd() {
    std::string s;
    {
        std::lock_guard<std::mutex> lock(g_cur_motd_mutex);
        s = g_cur_motd;
    }
    return s;
}

void GitHubChecker::OpenDZSimUpdatePageInBrowser()
{
    // @PORTING: Make this work not just on Windows

#ifdef DZSIM_WEB_PORT
    return; // TODO: Is opening a webpage possible on Emscripten?
#else
    // Open a website in the default browser (Windows-only)
    ShellExecute(NULL, "open", "https://github.com/lacyyy/DZSimulator/releases",
        NULL, NULL, SW_SHOWNORMAL);
#endif
}
