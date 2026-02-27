#pragma once

#include "Config.hpp"
#include <string>
#include <vector>
#include <SDL2/SDL.h>

class TextFile
{
public:
    TextFile();
    explicit TextFile(const std::string& filename);
    ~TextFile();

    bool Load(const std::string& filename);
    bool Save(const std::string& filename) const;
    void Clear();

    void AddLine(const std::string& line);
    void SetLine(size_t index, const std::string& line);
    void InsertLine(size_t index, const std::string& line);
    void RemoveLine(size_t index);

    std::string GetLine(size_t index) const;
    size_t GetLineCount() const { return m_lineCount; }
    bool IsEmpty() const { return m_lineCount == 0; }

 
    std::string GetAll() const;

    bool Contains(const std::string& text) const;
    size_t FindLine(const std::string& text, size_t startLine = 0) const;

    std::string GetValue(const std::string& key, const std::string& defaultValue = "") const;
    s32 GetInt(const std::string& key, s32 defaultValue = 0) const;
    f32 GetFloat(const std::string& key, f32 defaultValue = 0.0f) const;
    bool GetBool(const std::string& key, bool defaultValue = false) const;

    void SetValue(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, s32 value);
    void SetFloat(const std::string& key, f32 value);
    void SetBool(const std::string& key, bool value);

    std::vector<std::string> SplitLine(size_t index, char delimiter = ',') const;
    std::vector<std::string> GetSection(const std::string& sectionName) const;
    static std::string Trim(const std::string& str);
    static std::vector<std::string> Split(const std::string& str, char delimiter);

private:
    char** m_lines;
    size_t m_lineCount;
    size_t m_capacity;

    void Reserve(size_t capacity);
    void Grow();
};


enum class SeekOrigin : int
{
    Begin = 0,
    Current = 1,
    End = 2
};

class Stream
{
public:
    virtual ~Stream() = default;

    virtual size_t Read(void *buffer, size_t size) = 0;
    virtual size_t Write(const void *buffer, size_t size) = 0;
    virtual bool Seek(long offset, SeekOrigin origin = SeekOrigin::Begin) = 0;
    virtual long Tell() const = 0;
    virtual size_t Size() const = 0;
    virtual bool IsEOF() const = 0;
    virtual bool IsOpen() const = 0;
    virtual void Close() = 0;

    u8 ReadByte();
    bool ReadBool();
    s16 ReadShort();
    u16 ReadUShort();
    s32 ReadInt();
    u32 ReadUInt();
    s64 ReadLong();
    u64 ReadULong();
    f32 ReadFloat();
    f64 ReadDouble();

    void WriteByte(u8 value);
    void WriteBool(bool value);
    void WriteShort(s16 value);
    void WriteUShort(u16 value);
    void WriteInt(s32 value);
    void WriteUInt(u32 value);
    void WriteLong(s64 value);
    void WriteULong(u64 value);
    void WriteFloat(f32 value);
    void WriteDouble(f64 value);

    std::string ReadUTF();
    std::string ReadLine();
    std::string ReadString(size_t length);
    std::string ReadCString();
    std::string ReadAll();

    std::string  GetPath() const { return m_path; }

    void WriteUTF(const std::string &str);
    void WriteLine(const std::string &line);
    void WriteCString(const std::string &str);

    void SetBigEndian(bool bigEndian) { m_bigEndian = bigEndian; }
    bool IsBigEndian() const { return m_bigEndian; }

protected:
    bool m_bigEndian = false;

    u16 SwapU16(u16 value) const;
    u32 SwapU32(u32 value) const;
    u64 SwapU64(u64 value) const;

    u16 ReadU16();
    u32 ReadU32();
    u64 ReadU64();
    void WriteU16(u16 value);
    void WriteU32(u32 value);
    void WriteU64(u64 value);
    std::string m_path;
};

class FileStream : public Stream
{
public:
    FileStream();
    explicit FileStream(const std::string &filename, const std::string &mode = "rb");
    virtual ~FileStream();

    bool Open(const std::string &filename, const std::string &mode = "rb");
    virtual void Close() override;

    virtual size_t Read(void *buffer, size_t size) override;
    virtual size_t Write(const void *buffer, size_t size) override;
    virtual bool Seek(long offset, SeekOrigin origin = SeekOrigin::Begin) override;
    virtual long Tell() const override;
    virtual size_t Size() const override;
    virtual bool IsEOF() const override;
    virtual bool IsOpen() const override;

    const std::string &GetFilename() const { return m_filename; }

private:
    SDL_RWops *m_file;
    std::string m_filename;
    size_t m_size;
};

class MemoryStream : public Stream
{
public:
    MemoryStream();
    explicit MemoryStream(const void *data, size_t size, bool copy = true);
    explicit MemoryStream(size_t initialCapacity);
    virtual ~MemoryStream();

    virtual size_t Read(void *buffer, size_t size) override;
    virtual size_t Write(const void *buffer, size_t size) override;
    virtual bool Seek(long offset, SeekOrigin origin = SeekOrigin::Begin) override;
    virtual long Tell() const override;
    virtual size_t Size() const override;
    virtual bool IsEOF() const override;
    virtual bool IsOpen() const override { return m_data != nullptr; }
    virtual void Close() override { Clear(); }

    void Clear();
    void Reserve(size_t capacity);
    const u8 *GetData() const { return m_data; }
    u8 *GetDataMutable() { return m_data; }

private:
    u8 *m_data;
    size_t m_size;
    size_t m_capacity;
    size_t m_position;
    bool m_ownsMemory;

    void Grow(size_t minCapacity);
};
