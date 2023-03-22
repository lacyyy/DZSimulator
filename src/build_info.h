#ifndef BUILD_INFO_H_
#define BUILD_INFO_H_

#include <string>

namespace build_info {

    void print();

    const char* GetBuildTypeStr();
    const char* GetBuildTimeStr();
    const char* GetVersionStr(); // DZSimulator version string

    namespace thirdparty {

        const char* GetMagnumVersionStr();
        const char* GetMagnumPluginsVersionStr();
        const char* GetMagnumIntegrationVersionStr();
        const char* GetCorradeVersionStr();
        const char* GetSdlVersionStr();
        const char* GetImGuiVersionStr();
        const char* GetAsioVersionStr();
        const char* GetCppHttpLibVersionStr();
        const char* GetJsonVersionStr();
        const char* GetFsalVersionStr();
        const char* GetOpenSSLVersionStr();
        const char* GetJoltPhysicsVersionStr();


    }
    
}

#endif // BUILD_INFO_H_
