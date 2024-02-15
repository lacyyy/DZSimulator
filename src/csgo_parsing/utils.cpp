#include "csgo_parsing/utils.h"

#include <stdexcept>

using namespace csgo_parsing;


utils::RetCode::RetCode(bool success)
    : code(success ? SUCCESS : GENERIC_FAILURE), desc_msg("") {}
utils::RetCode::RetCode(int _code, const std::string& _desc_msg)
    : code(_code), desc_msg(_desc_msg) {}
utils::RetCode::RetCode(int _code, std::string&& _desc_msg)
    : code(_code), desc_msg(_desc_msg) {}

bool utils::RetCode::successful() const {
    return this->code == SUCCESS;
}
utils::RetCode::operator bool() const {
    return successful();
}


std::string utils::NormalizeGameFilePath(const std::string& str)
{
    std::string out;
    char last_c = '\0';
    for (char c : str) {
        if (c == '/' && last_c == '/')
            continue; // Remove duplicate forward slashes
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        out.push_back(c);
        last_c = c;
    }
    return out;
}

std::vector<std::string> utils::SplitString(const std::string& s, char delimiter)
{
    std::vector<std::string> out;

    size_t curr_pos = 0;
    while (1) {
        auto next_delimiter_pos = s.find(delimiter, curr_pos);
        if (next_delimiter_pos == std::string::npos)
            break;

        if (next_delimiter_pos > curr_pos) // if substring will be non-empty
            out.emplace_back(s.substr(curr_pos, next_delimiter_pos - curr_pos));
        curr_pos = next_delimiter_pos + 1;
    }
    // If we are not done yet, take string from curr_pos to the end of s
    if (curr_pos < s.size())
        out.emplace_back(s.substr(curr_pos));

    return out;
}

std::vector<float> utils::ParseFloatsFromString(const std::string& s)
{
    std::vector<std::string> substrings = utils::SplitString(s, ' ');
    std::vector<float> floats;
    floats.reserve(substrings.size());
    for (std::string& sub : substrings) {
        float result;
        try { result = std::stold(sub); }
        catch (const std::invalid_argument&) { result = 0.0f; }
        catch (const std::out_of_range&) { result = 0.0f; }
        floats.push_back(result);
    }
    return floats;
}

std::vector<int64_t> utils::ParseIntsFromString(const std::string& s)
{
    std::vector<std::string> substrings = utils::SplitString(s, ' ');
    std::vector<int64_t> ints;
    ints.reserve(substrings.size());
    for (std::string& sub : substrings) {
        int64_t result;
        try { result = std::stoll(sub); }
        catch (const std::invalid_argument&) { result = 0; }
        catch (const std::out_of_range&) { result = 0; }
        ints.push_back(result);
    }
    return ints;
}

float utils::ParseFloatFromString(const std::string& s, float default_val)
{
    auto float_list = utils::ParseFloatsFromString(s);
    if (float_list.size() == 0)
        return default_val;
    return float_list[0];
}

int64_t utils::ParseIntFromString(const std::string& s, int64_t default_val)
{
    auto int_list = utils::ParseIntsFromString(s);
    if (int_list.size() == 0)
        return default_val;
    return int_list[0];
}
