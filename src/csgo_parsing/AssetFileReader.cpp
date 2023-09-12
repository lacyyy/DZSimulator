#include "csgo_parsing/AssetFileReader.h"

#include <cmath>

#include <FileSystem.h>
#include <LockableFiles.h>
#include <SubFile.h>

using namespace csgo_parsing;

// IEEE 754 32bit float properties
#define F32_MANTISSA_SIZE 23
#define F32_EXPONENT_BIAS 127
#define F32_SIGN_SHIFT 31

struct AssetFileReader::Implementation {
    fsal::FileSystem fs; // Default-constructed singleton class
    fsal::File file;
    size_t pos = 0; // current read position, relative to beginning of file

    // LUT for float32 parsing
    double f32_mantissa_bit_pos_vals[F32_MANTISSA_SIZE];
};

AssetFileReader::AssetFileReader()
    // Default construct fsal::FileSystem and fsal::File members
    : _impl( new Implementation )
{
    // Precalculate LUT for float32 parsing
    for (int bit_pos = 0; bit_pos < F32_MANTISSA_SIZE; bit_pos++) {
        _impl->f32_mantissa_bit_pos_vals[bit_pos] =
            pow(2.0, bit_pos - F32_MANTISSA_SIZE);
    }
}

// Default destructor must be defined after AssetFileReader::Implementation was
// defined. Otherwise the default destructor would be defined in the header, not
// finding the definition of Implementation and refusing to compile.
AssetFileReader::~AssetFileReader() = default;

bool AssetFileReader::OpenFileFromAbsolutePath(const std::string& abs_file_path)
{
    // Only treat file path as absolute system path
    fsal::Location file_loc(abs_file_path, fsal::Location::kAbsolute);
    // File must be opened as a 'lockable' file to allow usage with fsal::SubFile
    _impl->file = _impl->fs.Open(file_loc, fsal::kRead, true);
    _impl->pos = 0;
    if (!_impl->file)
        return false;
    return true;
}

bool AssetFileReader::OpenFileFromGameFiles(const std::string& game_file_path)
{
    // Look for file in search paths and the indexed files from VPK archives
    // that were made available by calls to AssetFinder::FindCsgoPath() and
    // AssetFinder::RefreshVpkArchiveIndex()
    fsal::Location file_loc(game_file_path, fsal::Location::kSearchPathsAndArchives);
    // File must be opened as a 'lockable' file to allow usage with fsal::SubFile
    _impl->file = _impl->fs.Open(file_loc, fsal::kRead, true);
    _impl->pos = 0;
    if (!_impl->file)
        return false;
    return true;
}

bool AssetFileReader::OpenFileFromMemory(
    Corrade::Containers::ArrayView<const uint8_t> file_data)
{
    _impl->pos = 0;

    if (file_data.data() == nullptr) {
        _impl->file = fsal::File();
        return false;
    }
    else {
        // We need to use fsal::LMemRefFileReadOnly here (the lockable version
        // of fsal::MemRefFileReadOnly) in order to allow an fsal::SubFile to be
        // opened on top of it like in OpenSubFileFromCurrentlyOpenedFile() .
        // Technically, locking on read-only memory isn't necessary, but the
        // FSAL library works that way.
        _impl->file = fsal::File(new fsal::LMemRefFileReadOnly(
            file_data.data(),
            file_data.size()
        ));
        return true;
    }
}

bool AssetFileReader::OpenSubFileFromCurrentlyOpenedFile(
    size_t subfile_pos, size_t subfile_len)
{
    _impl->pos = 0;

    if (!IsOpenedInFile()) {
        _impl->file = fsal::File();
        return false;
    }

    auto currently_opened_file = _impl->file.GetInterface();
    _impl->file = fsal::File(new fsal::SubFile(
        currently_opened_file,
        subfile_len,
        subfile_pos
    ));
    return true;
}

bool AssetFileReader::IsOpenedInFile()
{
    return _impl->file;
}

size_t AssetFileReader::GetPos()
{
    return _impl->pos;
}

bool AssetFileReader::SetPos(size_t pos)
{
    if (!_impl->file)
        return false;

    auto op_status = _impl->file.Seek(pos, fsal::File::Origin::Beginning);
    _impl->pos = pos;
    return op_status.ok();
}

bool AssetFileReader::ReadByteArray(uint8_t* out, size_t len)
{
    if (!_impl->file)
        return false;

    auto op_status = _impl->file.Read(out, len);
    _impl->pos += len;
    return op_status.ok() && !op_status.is_eof();
}

bool AssetFileReader::ReadCharArray(char* out, size_t len)
{
    if (!_impl->file)
        return false;

    uint8_t c;
    while (len--) {
        auto op_status = _impl->file.Read(&c, 1);
        _impl->pos++;

        if (!op_status.ok() || op_status.is_eof())
            return false;

        *out = c; // uint8_t to char conversion
        out++;
    }
    return true;
}

bool AssetFileReader::ReadLine(std::string& out, uint8_t delim, size_t fail_len)
{
    out.clear();

    if (!_impl->file || fail_len == 0)
        return false;

    uint8_t c;
    size_t len = 0;
    while (1) {
        auto op_status = _impl->file.Read(&c, 1);
        _impl->pos++;

        if (!op_status.ok() || op_status.is_eof())
            return false;

        if (c == delim) // Don't include delimiter in output
            break;

        len++;
        if (len == fail_len)
            return false;

        out += c;
    }
    return true;
}

bool AssetFileReader::ReadUINT8(uint8_t& out)
{
    return ReadByteArray(&out, 1);
}

bool AssetFileReader::ReadUINT16_LE(uint16_t& out)
{
    uint8_t buf[2];
    if (!ReadByteArray(buf, 2))
        return false;

    out = buf[1] << 8 | buf[0];
    return true;
}

bool AssetFileReader::ReadUINT32_LE(uint32_t& out)
{
    uint8_t buf[4];
    if (!ReadByteArray(buf, 4))
        return false;

    out = buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
    return true;
}

bool AssetFileReader::ReadINT16_LE(int16_t& out)
{
    uint16_t u16;
    if (!ReadUINT16_LE(u16))
        return false;

    // check sign bit (2's complement)
    if (u16 & 0x8000) // if negative
        out = -(int16_t)(~u16) - 1; // I hope this is portable... (I'm too paranoid)
    else // if positive
        out = (int16_t)(u16);
    return true;
}

bool AssetFileReader::ReadINT32_LE(int32_t& out)
{
    uint32_t u32;
    if (!ReadUINT32_LE(u32))
        return false;

    // check sign bit (2's complement)
    if (u32 & 0x80000000) // if negative
        out = -(int32_t)(~u32) - 1; // I hope this is portable... (I'm too paranoid)
    else // if positive
        out = (int32_t)(u32);
    return true;
}

// Parses IEEE 754 32bit float from its little-endian binary form
bool AssetFileReader::ReadFLOAT32_LE(float& out)
{
    uint32_t fp_int;
    if (!ReadUINT32_LE(fp_int))
        return false;

    bool sign = fp_int >> F32_SIGN_SHIFT; // 1 bit
    int exponent = (fp_int >> F32_MANTISSA_SIZE) & 0xff; // 8 bits
    int mantissa = fp_int & 0x7fffff; // 23 bits

    // If number is denormalized
    bool is_denormalized = exponent == 0 && mantissa > 0;

    if (exponent == 0xff) {
        if (mantissa == 0) {
            out = sign ? -INFINITY : INFINITY;
        }
        else {
            if (std::numeric_limits<float>::has_quiet_NaN)
                out = std::numeric_limits<float>::quiet_NaN();
            else
                out = NAN;
        }
        return true;
    }
    else if (exponent == 0) {
        if (mantissa == 0) {
            out = sign ? -0.0f : 0.0f;
            return true;
        }
    }

    double total = is_denormalized ? 0.0 : 1.0;
    int bitPos = 0;
    while (mantissa) {
        if (mantissa & 0x01)
            total += _impl->f32_mantissa_bit_pos_vals[bitPos];

        mantissa >>= 1;
        bitPos++;
    }

    if (is_denormalized)
        exponent = 1 - F32_EXPONENT_BIAS;
    else
        exponent = exponent - F32_EXPONENT_BIAS;

    out = (sign ? -total : total) * pow(2.0, exponent);
    return true;
}
