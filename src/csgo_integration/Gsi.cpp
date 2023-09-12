#include "csgo_integration/Gsi.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <Corrade/Utility/Debug.h>
// Include httplib.h before Windows.h or include Windows.h by defining
// WIN32_LEAN_AND_MEAN beforehand.
#include <httplib.h>
#include <json.hpp>
#include <Magnum/Magnum.h>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace Magnum;
using json = nlohmann::json;

using namespace csgo_integration;

// PImpl programming technique, see header file
struct Gsi::impl {
    std::thread thread; // Listens for game data sent by CS:GO
    httplib::Server server;
    std::atomic<bool> is_stopping_server = false;
    std::atomic<bool> server_exited_unexpectedly = false;
    
    std::mutex packet_queue_mutex;
    std::queue<std::string> packet_queue;

    std::string auth_token;
};


Gsi::Gsi()
    : _pImpl{ std::make_unique<impl>() }
{}

Gsi::~Gsi()
{
    Stop();
}

bool Gsi::Start(const std::string& host, int port, const std::string& auth_token)
{
    if (IsRunning()) {
        throw std::logic_error("Gsi::Start() must not be called while "
            "Gsi::IsRunning() is true. Did you forget to call Gsi::Stop() ?");
    }

    _pImpl->is_stopping_server = false;
    _pImpl->server_exited_unexpectedly = false;
    _pImpl->packet_queue = {}; // Reset queue without mutex since thread is stopped
    _pImpl->auth_token = auth_token;

    auto http_handler = [this]
        (const httplib::Request& request, httplib::Response& response)
    {
        std::string req_body = request.body;
        {
            std::lock_guard<std::mutex> lock(_pImpl->packet_queue_mutex);
            _pImpl->packet_queue.push(std::move(req_body));
        }
        response.set_content("{}", "application/json");
        response.status = 200;
    };

    _pImpl->server.Post("/", http_handler);

    Debug{} << ("[GSI] Starting http server on " + host + ":"
        + std::to_string(port) + " ...").c_str();

    auto thread_start_time = steady_clock::now();

    _pImpl->thread = std::thread([this, host, port] {
        _pImpl->server.listen(host.c_str(), port);

        // Check if server exited expectedly
        if (!_pImpl->is_stopping_server) {
            _pImpl->server_exited_unexpectedly = true;
            Debug{} << "[GSI] Http server exited due to some error.";
        }
    });

    const long long MAX_STARTUP_TIME_MILLIS = 4000;
    while (!_pImpl->server.is_running()) {

        if (_pImpl->server_exited_unexpectedly)
            return false; // return failure

        long long millis_since_thread_start = duration_cast<milliseconds>(
            steady_clock::now() - thread_start_time).count();

        if (millis_since_thread_start > MAX_STARTUP_TIME_MILLIS) {
            // Why we throw in this scenario: The http server is currently
            // starting up, but isn't done with it yet: server.is_running() is
            // still false, it could become true shortly.
            // If we would now call server.stop(), it would do nothing because
            // server.is_running() still returns false. If we then try to join
            // the thread, we get stuck because the thread doesn't leave
            // server.listen(), either because it's still starting or
            // it has started and didn't get stopped by the stop call earlier!
            // Finding a way to fix that would be nice.
            throw std::logic_error("[GSI] httplib::Server is taking too long to"
                " start.");
        }

        std::this_thread::sleep_for(5ms);
    }

    Debug{} << "[GSI] Http server started within" << duration_cast<milliseconds>(
        steady_clock::now() - thread_start_time).count() / 1000.0f << "seconds";
    return true; // return success
}

void Gsi::Stop()
{
    if (!IsRunning())
        return;

    _pImpl->is_stopping_server = true;
    Debug{} << "[GSI] Stopping http server...";
    _pImpl->server.stop(); // Does nothing if server is already stopped
    if (_pImpl->thread.joinable())
        _pImpl->thread.join();
    Debug{} << "[GSI] Stopped!";
}

bool Gsi::IsRunning()
{
    return _pImpl->server.is_running() || _pImpl->thread.joinable();
}

bool Gsi::HasHttpServerUnexpectedlyClosed()
{
    return _pImpl->server_exited_unexpectedly;
}

std::vector<GsiState> Gsi::GetNewestGsiStates()
{
    // Quickly retrieve packet queue contents
    std::queue<std::string> packet_q;
    {
        std::lock_guard<std::mutex> lock(_pImpl->packet_queue_mutex);
        _pImpl->packet_queue.swap(packet_q);
    }

    // Parse gamestate data
    std::vector<GsiState> states;
    while (!packet_q.empty()) {
        GsiState s;
        s.json_payload = std::move(packet_q.front());
        packet_q.pop();

        // Parse without throwing exceptions
        json gamestate = json::parse(s.json_payload, nullptr, false);

        // If parse error occurred
        if (gamestate.is_discarded()) {
            Debug{} << "[GSI] JSON parse error! payload contents:";
            Debug{} << s.json_payload.c_str();
            Debug{} << "[GSI] end of payload contents";
            continue;
        }

        std::string payload_auth_token = ""; // Default token is no token

        // Iterate through json object
        for (auto& elem : gamestate.items()) {
            if (elem.value().is_object()) {
                // Get auth token value
                if (elem.key() == "auth") {
                    json& auth_obj = elem.value();
                    // Take the first string value we find, from whatever key
                    for (auto& e : auth_obj.items()) {
                        if (e.value().is_string()) {
                            payload_auth_token = e.value();
                            break;
                        }
                    }
                }
                // Get map name value
                if (elem.key() == "map") {
                    json& map_obj = elem.value();
                    for (auto& e : map_obj.items()) {
                        if (e.key() == "name" && e.value().is_string()) {
                            s.map_name = e.value();
                            break;
                        }
                    }
                }
                // Get player values
                if (elem.key() == "player") {
                    json& player_obj = elem.value();
                    for (auto& e : player_obj.items()) {
                        if (e.key() == "position" && e.value().is_string())
                            s.spec_pos = ParseGsiVector3(e.value());
                        else if (e.key() == "forward" && e.value().is_string())
                            s.spec_forward = ParseGsiVector3(e.value());
                    }
                }
            }
        }

        if (payload_auth_token != _pImpl->auth_token) {
            Debug{} << "[GSI] Received JSON payload with wrong auth token:"
                << payload_auth_token.c_str();
            continue;
        }

        states.push_back(std::move(s));
    }

    return states;
}

std::optional<Vector3> Gsi::ParseGsiVector3(std::string s)
{
    // Match separate float values with ONLY the following formats:
    // -0.14
    // 1.0
    // -782.1314
    std::regex regex(R"(-?[0-9]+\.[0-9]+)");
    auto vals_begin = std::sregex_iterator(s.begin(), s.end(), regex);
    auto vals_end   = std::sregex_iterator(); // end-of-sequence iterator

    std::vector<float> vals;
    for (std::sregex_iterator iter = vals_begin; iter != vals_end; ++iter) {
        // std::stof can't fail with used regex
        vals.push_back(std::stof(iter->str()));
    }
    if (vals.size() != 3)
        return std::optional<Vector3>(); // empty
    else
        return std::optional<Vector3>(std::in_place, vals[0], vals[1], vals[2]);
}
