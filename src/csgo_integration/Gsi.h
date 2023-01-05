#ifndef CSGO_INTEGRATION_GSI_H_
#define CSGO_INTEGRATION_GSI_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

namespace csgo_integration {

    struct GsiState {
        // Unparsed
        std::string json_payload;
        // Parsed
        std::optional<std::string> map_name;
        std::optional<Magnum::Vector3> spec_pos; // X, Y, Z
        std::optional<Magnum::Vector3> spec_forward;
    };

    // CSGO's Game State Integration
    class Gsi {
    public:
        Gsi();
        ~Gsi();

        // This method may only be called if IsRunning() returns false
        // @returns Whether or not the service was successfully started
        bool Start(const std::string& host, int port, const std::string& auth_token="");

        void Stop();

        bool IsRunning();
        bool HasHttpServerUnexpectedlyClosed();

        // Returns empty vector if nothing new is available
        std::vector<GsiState> GetNewestGsiStates();

    private:
        std::optional<Magnum::Vector3> ParseGsiVector3(std::string s);

        // Put private member variables into forward-declared struct that's
        // defined in the cpp to reduce size of this header file. (PImpl
        // programming technique) The main reason for that is usage of big
        // single-header http library in this class.
        struct impl;
        std::unique_ptr<impl> _pImpl;
    };

}

#endif // CSGO_INTEGRATION_GSI_H_
