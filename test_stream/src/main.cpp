

#include "Core.hpp"

#include <iostream>
#include <cassert>
#include <cmath>

#define TEST(name)                             \
    std::cout << "Testing " << name << "... "; \
    TestsPassed++
#define ASSERT_EQ(a, b)                                                                \
    if ((a) != (b))                                                                    \
    {                                                                                  \
        std::cout << "FAILED\n  Expected: " << (b) << "\n  Got: " << (a) << std::endl; \
        TestsFailed++;                                                                 \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
        std::cout << "OK" << std::endl;                                                \
    }
#define ASSERT_NEAR(a, b, eps)                                                         \
    if (fabs((a) - (b)) > (eps))                                                       \
    {                                                                                  \
        std::cout << "FAILED\n  Expected: " << (b) << "\n  Got: " << (a) << std::endl; \
        TestsFailed++;                                                                 \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
        std::cout << "OK" << std::endl;                                                \
    }
#define ASSERT_TRUE(cond)                   \
    if (!(cond))                            \
    {                                       \
        std::cout << "FAILED" << std::endl; \
        TestsFailed++;                      \
    }                                       \
    else                                    \
    {                                       \
        std::cout << "OK" << std::endl;     \
    }

int TestsPassed = 0;
int TestsFailed = 0;

void TestMemoryStreamBasic()
{
    MemoryStream ms(256);

    TEST("WriteByte");
    ms.WriteByte(42);
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_EQ(ms.ReadByte(), (u8)42);

    TEST("WriteInt");
    ms.Seek(0, SeekOrigin::Begin);
    ms.WriteInt(12345678);
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_EQ(ms.ReadInt(), (s32)12345678);

    TEST("WriteFloat");
    ms.Seek(0, SeekOrigin::Begin);
    ms.WriteFloat(3.14159f);
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_NEAR(ms.ReadFloat(), 3.14159f, 0.0001f);

    TEST("WriteDouble");
    ms.Seek(0, SeekOrigin::Begin);
    ms.WriteDouble(2.718281828);
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_NEAR(ms.ReadDouble(), 2.718281828, 0.000001);
}

void TestMemoryStreamStrings()
{
    MemoryStream ms(256);

    TEST("WriteUTF");
    ms.WriteUTF("Hello World");
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_EQ(ms.ReadUTF(), std::string("Hello World"));

    TEST("WriteCString");
    ms.Seek(0, SeekOrigin::Begin);
    ms.WriteCString("Test");
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_EQ(ms.ReadCString(), std::string("Test"));

    TEST("WriteLine");
    ms.Seek(0, SeekOrigin::Begin);
    ms.WriteLine("Line1");
    ms.WriteLine("Line2");
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_EQ(ms.ReadLine(), std::string("Line1"));
}

void TestMemoryStreamSeek()
{
    MemoryStream ms(256);

    ms.WriteInt(1);
    ms.WriteInt(2);
    ms.WriteInt(3);

    TEST("Seek Begin");
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_EQ(ms.ReadInt(), (s32)1);

    TEST("Seek Current");
    ms.Seek(-4, SeekOrigin::Current);
    ASSERT_EQ(ms.ReadInt(), (s32)1);

    TEST("Seek End");
    ms.Seek(-4, SeekOrigin::End);
    ASSERT_EQ(ms.ReadInt(), (s32)3);

    TEST("Tell");
    ms.Seek(0, SeekOrigin::Begin);
    ms.ReadInt();
    ASSERT_EQ(ms.Tell(), (long)4);
}

void TestMemoryStreamGrow()
{
    MemoryStream ms(4);

    TEST("Auto grow");
    for (int i = 0; i < 1000; i++)
        ms.WriteInt(i);

    ms.Seek(0, SeekOrigin::Begin);
    bool allCorrect = true;
    for (int i = 0; i < 1000; i++)
    {
        if (ms.ReadInt() != i)
        {
            allCorrect = false;
            break;
        }
    }
    ASSERT_TRUE(allCorrect);
}

void TestMemoryStreamEndianness()
{
    MemoryStream ms(16);

    TEST("Little Endian (default)");
    ms.SetBigEndian(false);
    ms.WriteUInt(0x12345678);
    ms.Seek(0, SeekOrigin::Begin);
    u8 bytes[4];
    ms.Read(bytes, 4);
    bool littleEndian = (bytes[0] == 0x78 && bytes[1] == 0x56 && bytes[2] == 0x34 && bytes[3] == 0x12);
    ASSERT_TRUE(littleEndian);

    TEST("Big Endian");
    ms.Seek(0, SeekOrigin::Begin);
    ms.SetBigEndian(true);
    ms.WriteUInt(0x12345678);
    ms.Seek(0, SeekOrigin::Begin);
    ms.Read(bytes, 4);
    bool bigEndian = (bytes[0] == 0x12 && bytes[1] == 0x34 && bytes[2] == 0x56 && bytes[3] == 0x78);
    ASSERT_TRUE(bigEndian);
}

void TestMemoryStreamWrap()
{
    u8 buffer[64];
    for (int i = 0; i < 64; i++)
        buffer[i] = i;

    TEST("Wrap external memory (no copy)");
    MemoryStream ms(buffer, 64, false);
    ASSERT_EQ(ms.ReadByte(), (u8)0);
    ASSERT_EQ(ms.ReadByte(), (u8)1);
    ASSERT_EQ(ms.ReadByte(), (u8)2);
}

void TestMemoryStreamCopy()
{
    u8 buffer[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    TEST("Copy external memory");
    MemoryStream ms(buffer, 16, true);
    buffer[0] = 99;
    ms.Seek(0, SeekOrigin::Begin);
    ASSERT_EQ(ms.ReadByte(), (u8)1);
}

void TestFileStream()
{
    const char *testFile = "test_stream.bin";

    TEST("FileStream Write");
    {
        FileStream fs(testFile, "wb");
        bool opened = fs.IsOpen();
        if (opened)
        {
            fs.WriteInt(123456);
            fs.WriteFloat(3.14f);
            fs.WriteUTF("FileTest");
            fs.Close();
        }
        ASSERT_TRUE(opened);
    }

    TEST("FileStream Read");
    {
        FileStream fs(testFile, "rb");
        bool dataCorrect = false;
        if (fs.IsOpen())
        {
            s32 i = fs.ReadInt();
            f32 f = fs.ReadFloat();
            std::string s = fs.ReadUTF();
            dataCorrect = (i == 123456 && fabs(f - 3.14f) < 0.001f && s == "FileTest");
            fs.Close();
        }
        ASSERT_TRUE(dataCorrect);
    }

    TEST("FileStream Size");
    {
        FileStream fs(testFile, "rb");
        size_t size = fs.Size();
        fs.Close();
        ASSERT_EQ(size, (size_t)(4 + 4 + 2 + 8));
    }

    SDL_RWops *rw = SDL_RWFromFile(testFile, "rb");
    if (rw)
    {
        SDL_RWclose(rw);
        remove(testFile);
    }
}

void TestEdgeCases()
{
    TEST("Empty MemoryStream");
    MemoryStream ms1;
    ASSERT_TRUE(ms1.IsEOF());

    TEST("Read beyond EOF");
    MemoryStream ms2(4);
    ms2.WriteInt(1);
    ms2.Seek(0, SeekOrigin::Begin);
    ms2.ReadInt();
    ASSERT_EQ(ms2.ReadByte(), (u8)0);

    TEST("Invalid Seek");
    MemoryStream ms3(16);
    ms3.WriteInt(1);
    bool seekFailed = !ms3.Seek(100, SeekOrigin::Begin);
    ASSERT_TRUE(seekFailed);

    TEST("Clear");
    MemoryStream ms4(64);
    ms4.WriteInt(123);
    ms4.Clear();
    ASSERT_EQ(ms4.Size(), (size_t)0);
}

void TestAllTypes()
{
    MemoryStream ms(256);

    ms.WriteByte(255);
    ms.WriteBool(true);
    ms.WriteShort(-32768);
    ms.WriteUShort(65535);
    ms.WriteInt(-2147483648);
    ms.WriteUInt(4294967295U);
    ms.WriteLong(-9223372036854775807LL);
    ms.WriteULong(18446744073709551615ULL);
    ms.WriteFloat(1.23456f);
    ms.WriteDouble(9.87654321);

    ms.Seek(0, SeekOrigin::Begin);

    TEST("All types round-trip");
    bool allCorrect = true;
    allCorrect &= (ms.ReadByte() == 255);
    allCorrect &= (ms.ReadBool() == true);
    allCorrect &= (ms.ReadShort() == -32768);
    allCorrect &= (ms.ReadUShort() == 65535);
    allCorrect &= (ms.ReadInt() == -2147483648);
    allCorrect &= (ms.ReadUInt() == 4294967295U);
    allCorrect &= (ms.ReadLong() == -9223372036854775807LL);
    allCorrect &= (ms.ReadULong() == 18446744073709551615ULL);
    allCorrect &= (fabs(ms.ReadFloat() - 1.23456f) < 0.0001f);
    allCorrect &= (fabs(ms.ReadDouble() - 9.87654321) < 0.000001);
    ASSERT_TRUE(allCorrect);
}

int main()
{
    std::cout << "=== Stream Test Suite ===" << std::endl
              << std::endl;

    TestMemoryStreamBasic();
    TestMemoryStreamStrings();
    TestMemoryStreamSeek();
    TestMemoryStreamGrow();
    TestMemoryStreamEndianness();
    TestMemoryStreamWrap();
    TestMemoryStreamCopy();
    TestFileStream();
    TestEdgeCases();
    TestAllTypes();

    std::cout << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << "Tests Passed: " << TestsPassed << std::endl;
    std::cout << "Tests Failed: " << TestsFailed << std::endl;
    std::cout << "==========================" << std::endl;

    // Config file (config.ini)
    TextFile config("config.ini");
    int width = config.GetInt("width", 800);
    int height = config.GetInt("height", 600);
    bool fullscreen = config.GetBool("fullscreen", false);

    config.SetInt("width", 1024);
    config.SetBool("fullscreen", true);
    config.Save("config.ini");

    // CSV parsing
    TextFile csv("data.csv");
    for (size_t i = 0; i < csv.GetLineCount(); i++)
    {
        auto fields = csv.SplitLine(i, ',');
        if (fields.size() >= 3)
        {
            std::string name = fields[0];
            int age = atoi(fields[1].c_str());
            std::string city = fields[2];
            LogInfo("Name: %s, Age: %d, City: %s", name.c_str(), age, city.c_str());
        }
    }

    // INI sections
    TextFile ini("settings.ini");
    auto graphics = ini.GetSection("Graphics");
    for (const auto &line : graphics)
    {
        // Process graphics settings
    }

    // Log file
    TextFile log;
    log.AddLine("Application started");
    log.AddLine("Loading resources...");
    log.Save("app.log");

    return TestsFailed > 0 ? 1 : 0;
}
