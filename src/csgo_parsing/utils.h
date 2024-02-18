#ifndef CSGO_PARSING_UTILS_H_
#define CSGO_PARSING_UTILS_H_

#include <string>
#include <vector>

namespace csgo_parsing::utils {

    // Returned object indicating a parse operation's success or failure
    class RetCode {
    public:
        // MEMBER FIELDS
        int code;
        std::string desc_msg; // Description of return code

        // CTORS / METHODS
        RetCode(bool success);
        RetCode(int code = SUCCESS, const std::string& desc_msg = "");
        RetCode(int code, std::string&& desc_msg);
        bool successful() const; // If RetCode object indicates success
        operator bool() const; // If RetCode object indicates success

        // RETURN CODE VALUES
        enum {
            // Some return codes have description strings, some don't
            SUCCESS = 0,               // possibly w/ desc, Operation succeeded
            GENERIC_FAILURE,           // no desc, Operation failed
            STEAM_NOT_INSTALLED,       // no desc, Steam is not installed
            CSGO_NOT_INSTALLED,        // no desc, CSGO is not installed
            ERROR_STEAM_REGISTRY,      // w/ desc, Windows registry error
            ERROR_FILE_OPEN_FAILED,    // w/ desc, File open operation failed
            ERROR_VPK_PARSING_FAILED,  // no desc, Failed to open VPK Archive
            ERROR_BSP_PARSING_FAILED,  // w/ desc, Failed to parse BSP file
            ERROR_PHY_PARSING_FAILED,  // w/ desc, Failed to parse PHY file
            ERROR_PHY_MULTIPLE_SOLIDS, // no desc, PHY had multiple solids
        };
    };

    // Convert A-Z characters to lower case and remove duplicate slashes
    // Note: CSGO looks up game files irrespective of upper/lower case
    std::string NormalizeGameFilePath(const std::string& p);

    // Does not return empty substrings
    std::vector<std::string> SplitString(const std::string& s, char delimiter = ' ');

    std::vector<float> ParseFloatsFromString(const std::string& s);
    std::vector<int64_t> ParseIntsFromString(const std::string& s);

    float ParseFloatFromString(const std::string& s, float default_val = NAN);
    int64_t ParseIntFromString(const std::string& s, int64_t default_val = 0);

}

#endif // CSGO_PARSING_UTILS_H_
