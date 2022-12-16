#ifndef GITHUBCHECKER_H_
#define GITHUBCHECKER_H_

#include <string>

// Only one GitHubChecker instance may exist at once!
class GitHubChecker {
public:
    GitHubChecker();
    ~GitHubChecker(); // Might block (if async check is not finished)

    // ---- UPDATE AND MOTD (MESSAGE OF THE DAY) CHECKING
    void StartAsyncUpdateAndMotdCheck();
    bool IsAsyncUpdateAndMotdCheckFinished();

    enum UpdateStatus {
        NOT_CHECKED,
        NO_UPDATE_AVAILABLE,
        UPDATE_AVAILABLE,
        UPDATE_CHECK_FAILED
    };
    UpdateStatus GetUpdateStatus();

    std::string GetMotd();


    // ---- MISC
    static void OpenDZSimUpdatePageInBrowser();

};

#endif // GITHUBCHECKER_H_
