#include "pch.h"
#include "Stream.hpp"
#include "Utils.hpp"


 

TextFile::TextFile()
    : m_lines(nullptr), m_lineCount(0), m_capacity(0)
{
}

TextFile::TextFile(const std::string& filename)
    : m_lines(nullptr), m_lineCount(0), m_capacity(0)
{
    Load(filename);
}

TextFile::~TextFile()
{
    Clear();
}

bool TextFile::Load(const std::string& filename)
{
    Clear();
    
    SDL_RWops* file = SDL_RWFromFile(filename.c_str(), "rb");
    if (!file) return false;
    
    Sint64 size = SDL_RWsize(file);
    if (size <= 0)
    {
        SDL_RWclose(file);
        return false;
    }
    
    char* buffer = static_cast<char*>(malloc(size + 1));
    if (!buffer)
    {
        SDL_RWclose(file);
        return false;
    }
    
    SDL_RWread(file, buffer, 1, size);
    buffer[size] = '\0';
    SDL_RWclose(file);
    
    char* line = buffer;
    char* end = buffer + size;
    
    while (line < end)
    {
        char* lineEnd = line;
        while (lineEnd < end && *lineEnd != '\n' && *lineEnd != '\r')
            lineEnd++;
        
        size_t lineLength = lineEnd - line;
        if (lineLength > 0 || lineEnd < end)
        {
            std::string lineStr(line, lineLength);
            AddLine(lineStr);
        }
        
        line = lineEnd;
        if (line < end && *line == '\r') line++;
        if (line < end && *line == '\n') line++;
    }
    
    free(buffer);
    return true;
}

bool TextFile::Save(const std::string& filename) const
{
    SDL_RWops* file = SDL_RWFromFile(filename.c_str(), "wb");
    if (!file) return false;
    
    for (size_t i = 0; i < m_lineCount; i++)
    {
        SDL_RWwrite(file, m_lines[i], 1, strlen(m_lines[i]));
        SDL_RWwrite(file, "\n", 1, 1);
    }
    
    SDL_RWclose(file);
    return true;
}

void TextFile::Clear()
{
    if (m_lines)
    {
        for (size_t i = 0; i < m_lineCount; i++)
            free(m_lines[i]);
        free(m_lines);
    }
    m_lines = nullptr;
    m_lineCount = 0;
    m_capacity = 0;
}

void TextFile::AddLine(const std::string& line)
{
    if (m_lineCount >= m_capacity)
        Grow();
    
    m_lines[m_lineCount] = static_cast<char*>(malloc(line.length() + 1));
    strcpy(m_lines[m_lineCount], line.c_str());
    m_lineCount++;
}

void TextFile::SetLine(size_t index, const std::string& line)
{
    if (index >= m_lineCount) return;
    
    free(m_lines[index]);
    m_lines[index] = static_cast<char*>(malloc(line.length() + 1));
    strcpy(m_lines[index], line.c_str());
}

void TextFile::InsertLine(size_t index, const std::string& line)
{
    if (index > m_lineCount) index = m_lineCount;
    
    if (m_lineCount >= m_capacity)
        Grow();
    
    for (size_t i = m_lineCount; i > index; i--)
        m_lines[i] = m_lines[i - 1];
    
    m_lines[index] = static_cast<char*>(malloc(line.length() + 1));
    strcpy(m_lines[index], line.c_str());
    m_lineCount++;
}

void TextFile::RemoveLine(size_t index)
{
    if (index >= m_lineCount) return;
    
    free(m_lines[index]);
    
    for (size_t i = index; i < m_lineCount - 1; i++)
        m_lines[i] = m_lines[i + 1];
    
    m_lineCount--;
}

std::string TextFile::GetLine(size_t index) const
{
    if (index >= m_lineCount) return "";
    return std::string(m_lines[index]);
}

std::string TextFile::GetAll() const
{
    std::string result;
    for (size_t i = 0; i < m_lineCount; i++)
    {
        result += m_lines[i];
        if (i < m_lineCount - 1)
            result += '\n';
    }
    return result;
}

bool TextFile::Contains(const std::string& text) const
{
    return FindLine(text) != static_cast<size_t>(-1);
}

size_t TextFile::FindLine(const std::string& text, size_t startLine) const
{
    for (size_t i = startLine; i < m_lineCount; i++)
    {
        if (strstr(m_lines[i], text.c_str()) != nullptr)
            return i;
    }
    return static_cast<size_t>(-1);
}

std::string TextFile::GetValue(const std::string& key, const std::string& defaultValue) const
{
    std::string searchKey = key + "=";
    for (size_t i = 0; i < m_lineCount; i++)
    {
        std::string line = Trim(m_lines[i]);
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        
        if (line.find(searchKey) == 0)
        {
            size_t pos = line.find('=');
            if (pos != std::string::npos)
                return Trim(line.substr(pos + 1));
        }
    }
    return defaultValue;
}

s32 TextFile::GetInt(const std::string& key, s32 defaultValue) const
{
    std::string value = GetValue(key, "");
    if (value.empty()) return defaultValue;
    return atoi(value.c_str());
}

f32 TextFile::GetFloat(const std::string& key, f32 defaultValue) const
{
    std::string value = GetValue(key, "");
    if (value.empty()) return defaultValue;
    return static_cast<f32>(atof(value.c_str()));
}

bool TextFile::GetBool(const std::string& key, bool defaultValue) const
{
    std::string value = Trim(GetValue(key, ""));
    if (value.empty()) return defaultValue;
    
    if (value == "true" || value == "1" || value == "yes" || value == "on")
        return true;
    if (value == "false" || value == "0" || value == "no" || value == "off")
        return false;
    
    return defaultValue;
}

void TextFile::SetValue(const std::string& key, const std::string& value)
{
    std::string searchKey = key + "=";
    for (size_t i = 0; i < m_lineCount; i++)
    {
        std::string line = Trim(m_lines[i]);
        if (line.find(searchKey) == 0)
        {
            SetLine(i, key + "=" + value);
            return;
        }
    }
    AddLine(key + "=" + value);
}

void TextFile::SetInt(const std::string& key, s32 value)
{
    SetValue(key, std::to_string(value));
}

void TextFile::SetFloat(const std::string& key, f32 value)
{
    SetValue(key, std::to_string(value));
}

void TextFile::SetBool(const std::string& key, bool value)
{
    SetValue(key, value ? "true" : "false");
}

std::vector<std::string> TextFile::SplitLine(size_t index, char delimiter) const
{
    if (index >= m_lineCount) return {};
    return Split(m_lines[index], delimiter);
}

std::vector<std::string> TextFile::GetSection(const std::string& sectionName) const
{
    std::vector<std::string> result;
    std::string sectionHeader = "[" + sectionName + "]";
    bool inSection = false;
    
    for (size_t i = 0; i < m_lineCount; i++)
    {
        std::string line = Trim(m_lines[i]);
        
        if (line == sectionHeader)
        {
            inSection = true;
            continue;
        }
        
        if (inSection)
        {
            if (!line.empty() && line[0] == '[')
                break;
            result.push_back(line);
        }
    }
    
    return result;
}

void TextFile::Reserve(size_t capacity)
{
    if (capacity <= m_capacity) return;
    
    char** newLines = static_cast<char**>(realloc(m_lines, capacity * sizeof(char*)));
    if (!newLines) return;
    
    m_lines = newLines;
    m_capacity = capacity;
}

void TextFile::Grow()
{
    size_t newCapacity = m_capacity == 0 ? 16 : m_capacity * 2;
    Reserve(newCapacity);
}

std::string TextFile::Trim(const std::string& str)
{
    size_t start = 0;
    while (start < str.length() && (str[start] == ' ' || str[start] == '\t'))
        start++;
    
    size_t end = str.length();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\r'))
        end--;
    
    return str.substr(start, end - start);
}

std::vector<std::string> TextFile::Split(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos)
    {
        result.push_back(Trim(str.substr(start, end - start)));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    
    result.push_back(Trim(str.substr(start)));
    return result;
}


u16 Stream::SwapU16(u16 value) const
{
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

u32 Stream::SwapU32(u32 value) const
{
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x000000FF) << 24);
}

u64 Stream::SwapU64(u64 value) const
{
    return ((value & 0xFF00000000000000ULL) >> 56) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x000000FF00000000ULL) >> 8) |
           ((value & 0x00000000FF000000ULL) << 8) |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x00000000000000FFULL) << 56);
}

u16 Stream::ReadU16()
{
    u16 value;
    Read(&value, sizeof(u16));
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    if (!m_bigEndian) value = SwapU16(value);
#else
    if (m_bigEndian) value = SwapU16(value);
#endif
    return value;
}

u32 Stream::ReadU32()
{
    u32 value;
    Read(&value, sizeof(u32));
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    if (!m_bigEndian) value = SwapU32(value);
#else
    if (m_bigEndian) value = SwapU32(value);
#endif
    return value;
}

u64 Stream::ReadU64()
{
    u64 value;
    Read(&value, sizeof(u64));
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    if (!m_bigEndian) value = SwapU64(value);
#else
    if (m_bigEndian) value = SwapU64(value);
#endif
    return value;
}

void Stream::WriteU16(u16 value)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    if (!m_bigEndian) value = SwapU16(value);
#else
    if (m_bigEndian) value = SwapU16(value);
#endif
    Write(&value, sizeof(u16));
}

void Stream::WriteU32(u32 value)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    if (!m_bigEndian) value = SwapU32(value);
#else
    if (m_bigEndian) value = SwapU32(value);
#endif
    Write(&value, sizeof(u32));
}

void Stream::WriteU64(u64 value)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    if (!m_bigEndian) value = SwapU64(value);
#else
    if (m_bigEndian) value = SwapU64(value);
#endif
    Write(&value, sizeof(u64));
}

u8 Stream::ReadByte()
{
    u8 value;
    Read(&value, 1);
    return value;
}

bool Stream::ReadBool()
{
    return ReadByte() != 0;
}

s16 Stream::ReadShort()
{
    return static_cast<s16>(ReadU16());
}

u16 Stream::ReadUShort()
{
    return ReadU16();
}

s32 Stream::ReadInt()
{
    return static_cast<s32>(ReadU32());
}

u32 Stream::ReadUInt()
{
    return ReadU32();
}

s64 Stream::ReadLong()
{
    return static_cast<s64>(ReadU64());
}

u64 Stream::ReadULong()
{
    return ReadU64();
}

f32 Stream::ReadFloat()
{
    u32 temp = ReadU32();
    f32 value;
    memcpy(&value, &temp, sizeof(f32));
    return value;
}

f64 Stream::ReadDouble()
{
    u64 temp = ReadU64();
    f64 value;
    memcpy(&value, &temp, sizeof(f64));
    return value;
}

void Stream::WriteByte(u8 value)
{
    Write(&value, 1);
}

void Stream::WriteBool(bool value)
{
    WriteByte(value ? 1 : 0);
}

void Stream::WriteShort(s16 value)
{
    WriteU16(static_cast<u16>(value));
}

void Stream::WriteUShort(u16 value)
{
    WriteU16(value);
}

void Stream::WriteInt(s32 value)
{
    WriteU32(static_cast<u32>(value));
}

void Stream::WriteUInt(u32 value)
{
    WriteU32(value);
}

void Stream::WriteLong(s64 value)
{
    WriteU64(static_cast<u64>(value));
}

void Stream::WriteULong(u64 value)
{
    WriteU64(value);
}

void Stream::WriteFloat(f32 value)
{
    u32 temp;
    memcpy(&temp, &value, sizeof(f32));
    WriteU32(temp);
}

void Stream::WriteDouble(f64 value)
{
    u64 temp;
    memcpy(&temp, &value, sizeof(f64));
    WriteU64(temp);
}

std::string Stream::ReadUTF()
{
    u16 length = ReadUShort();
    if (length == 0) return "";
    std::string str;
    str.resize(length);
    Read(&str[0], length);
    return str;
}

std::string Stream::ReadLine()
{
    std::string line;
    char ch;
    while (Read(&ch, 1) == 1 && ch != '\n')
    {
        if (ch != '\r') line += ch;
    }
    return line;
}

std::string Stream::ReadString(size_t length)
{
    std::string str;
    str.resize(length);
    size_t read = Read(&str[0], length);
    str.resize(read);
    return str;
}

std::string Stream::ReadCString()
{
    std::string str;
    char ch;
    while (Read(&ch, 1) == 1 && ch != '\0')
    {
        str += ch;
    }
    return str;
}

std::string Stream::ReadAll()
{
    size_t size = Size();
    size_t current = Tell();
    Seek(0, SeekOrigin::Begin);
    std::string content = ReadString(size);
    Seek(current, SeekOrigin::Begin);
    return content;
}

void Stream::WriteUTF(const std::string &str)
{
    u16 length = static_cast<u16>(str.length());
    WriteUShort(length);
    Write(str.c_str(), length);
}

void Stream::WriteLine(const std::string &line)
{
    Write(line.c_str(), line.size());
    WriteByte('\n');
}

void Stream::WriteCString(const std::string &str)
{
    Write(str.c_str(), str.size());
    WriteByte('\0');
}

FileStream::FileStream() : m_file(nullptr), m_size(0) {}

FileStream::FileStream(const std::string &filename, const std::string &mode)
    : m_file(nullptr), m_size(0)
{
    Open(filename, mode);
}

FileStream::~FileStream()
{
    Close();
}

bool FileStream::Open(const std::string &filename, const std::string &mode)
{
    Close();
    m_filename = filename;
    m_path = Utils::GetDirectoryPath(filename.c_str());
    m_file = SDL_RWFromFile(filename.c_str(), mode.c_str());
    if (!m_file) return false;
    Sint64 size = SDL_RWsize(m_file);
    m_size = (size >= 0) ? static_cast<size_t>(size) : 0;
    return true;
}

void FileStream::Close()
{
    if (m_file)
    {
        SDL_RWclose(m_file);
        m_file = nullptr;
    }
    m_size = 0;
}

size_t FileStream::Read(void *buffer, size_t size)
{
    if (!m_file) return 0;
    return SDL_RWread(m_file, buffer, 1, size);
}

size_t FileStream::Write(const void *buffer, size_t size)
{
    if (!m_file) return 0;
    return SDL_RWwrite(m_file, buffer, 1, size);
}

bool FileStream::Seek(long offset, SeekOrigin origin)
{
    if (!m_file) return false;
    int whence = RW_SEEK_SET;
    switch (origin)
    {
    case SeekOrigin::Begin: whence = RW_SEEK_SET; break;
    case SeekOrigin::Current: whence = RW_SEEK_CUR; break;
    case SeekOrigin::End: whence = RW_SEEK_END; break;
    }
    return SDL_RWseek(m_file, offset, whence) >= 0;
}

long FileStream::Tell() const
{
    if (!m_file) return -1;
    return static_cast<long>(SDL_RWtell(m_file));
}

size_t FileStream::Size() const
{
    return m_size;
}

bool FileStream::IsEOF() const
{
    if (!m_file) return true;
    return Tell() >= static_cast<long>(m_size);
}

bool FileStream::IsOpen() const
{
    return m_file != nullptr;
}

MemoryStream::MemoryStream()
    : m_data(nullptr), m_size(0), m_capacity(0), m_position(0), m_ownsMemory(true)
{
}

MemoryStream::MemoryStream(const void *data, size_t size, bool copy)
    : m_data(nullptr), m_size(size), m_capacity(size), m_position(0), m_ownsMemory(copy)
{
    if (copy)
    {
        m_data = static_cast<u8*>(malloc(size));
        if (m_data && data && size > 0)
            memcpy(m_data, data, size);
    }
    else
    {
        m_data = const_cast<u8*>(static_cast<const u8*>(data));
    }
}

MemoryStream::MemoryStream(size_t initialCapacity)
    : m_data(nullptr), m_size(0), m_capacity(0), m_position(0), m_ownsMemory(true)
{
    if (initialCapacity > 0)
        Reserve(initialCapacity);
}

MemoryStream::~MemoryStream()
{
    Clear();
}

size_t MemoryStream::Read(void *buffer, size_t size)
{
    if (!m_data || m_position >= m_size) return 0;
    size_t available = m_size - m_position;
    size_t toRead = (size < available) ? size : available;
    memcpy(buffer, m_data + m_position, toRead);
    m_position += toRead;
    return toRead;
}

size_t MemoryStream::Write(const void *buffer, size_t size)
{
    if (m_position + size > m_capacity)
        Grow(m_position + size);
    
    memcpy(m_data + m_position, buffer, size);
    m_position += size;
    
    if (m_position > m_size)
        m_size = m_position;
    
    return size;
}

bool MemoryStream::Seek(long offset, SeekOrigin origin)
{
    long newPos = 0;
    switch (origin)
    {
    case SeekOrigin::Begin: newPos = offset; break;
    case SeekOrigin::Current: newPos = static_cast<long>(m_position) + offset; break;
    case SeekOrigin::End: newPos = static_cast<long>(m_size) + offset; break;
    }
    if (newPos < 0 || static_cast<size_t>(newPos) > m_size) return false;
    m_position = static_cast<size_t>(newPos);
    return true;
}

long MemoryStream::Tell() const
{
    return static_cast<long>(m_position);
}

size_t MemoryStream::Size() const
{
    return m_size;
}

bool MemoryStream::IsEOF() const
{
    return m_position >= m_size;
}

void MemoryStream::Clear()
{
    if (m_data && m_ownsMemory)
    {
        free(m_data);
    }
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
    m_position = 0;
}

void MemoryStream::Reserve(size_t capacity)
{
    if (capacity <= m_capacity) return;
    
    u8 *newData = static_cast<u8*>(realloc(m_data, capacity));
    if (!newData) return;
    
    m_data = newData;
    m_capacity = capacity;
}

void MemoryStream::Grow(size_t minCapacity)
{
    size_t newCapacity = m_capacity == 0 ? 64 : m_capacity;
    while (newCapacity < minCapacity)
        newCapacity *= 2;
    Reserve(newCapacity);
}
