// Platform abstraction layer
// Provides Unreal Engine types in standalone mode, uses UE types when integrated
// This allows seamless copy-paste between standalone and Unreal projects

#pragma once

// Detect if we're building standalone or in Unreal Engine
#ifdef STANDALONE_BUILD

// Standalone mode: Provide UE-like types using STL
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <cstdlib>

// Windows headers (before everything else)
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    // Undefine Windows macros that conflict with script language keywords
    #undef TEXT
    #undef TCHAR
    #undef CONST
    #undef VOID
    #undef INT
    #undef FLOAT
    #undef PUBLIC
    #undef PRIVATE
    #undef ERROR
    #undef constant
#endif

// Basic type aliases (must be defined before use)
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

// Unreal Engine macros for standalone
#define UENUM()
#define UCLASS()
#define UPROPERTY(...)
#define UFUNCTION(...)
#define USTRUCT()
#define GENERATED_BODY()

// Logging macros for standalone
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define LOG_WARNING(msg) std::cout << "[WARNING] " << msg << std::endl
#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl
#define UE_LOG(Category, Verbosity, Format, ...) std::cout << "[" #Verbosity "] " << Format << std::endl

// Search case enum (defined early for string functions)
enum class ESearchCase
{
    CaseSensitive,
    IgnoreCase
};

// String type wrapper to support UE-style dereferencing (*FString returns c_str())
class FString : public std::string
{
public:
    using std::string::string;
    using std::string::operator=;
    
    // Allow construction from std::string
    FString(const std::string& str) : std::string(str) {}
    FString(std::string&& str) : std::string(std::move(str)) {}
    
    // UE-style dereference operator (returns c_str())
    const char* operator*() const { return this->c_str(); }
    
    // Equals method for UE compatibility (overloaded for ESearchCase)
    bool Equals(const FString& other, bool caseSensitive = true) const
    {
        if (caseSensitive)
            return *this == other;
        else
        {
            FString a = *this, b = other;
            std::transform(a.begin(), a.end(), a.begin(), ::tolower);
            std::transform(b.begin(), b.end(), b.begin(), ::tolower);
            return a == b;
        }
    }
    
    bool Equals(const FString& other, ESearchCase searchCase) const
    {
        return Equals(other, searchCase == ESearchCase::CaseSensitive);
    }
    
    // Left - returns leftmost N characters
    FString Left(int32 count) const
    {
        if (count <= 0) return FString();
        return FString(this->substr(0, static_cast<size_t>(count)));
    }
    
    // Contains - checks if string contains substring
    bool Contains(const FString& substring) const
    {
        return this->find(substring) != std::string::npos;
    }
    
    // IsEmpty - checks if string is empty
    bool IsEmpty() const
    {
        return this->empty();
    }
    
    // SanitizeFloat - removes trailing zeros from float strings
    static FString SanitizeFloat(double value)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.10f", value);
        FString result(buffer);
        // Remove trailing zeros
        size_t pos = result.find_last_not_of('0');
        if (pos != std::string::npos && result[pos] != '.')
            result.erase(pos + 1);
        else if (pos != std::string::npos && result[pos] == '.')
            result.erase(pos);
        return result;
    }
    
    // Len - returns length
    int32 Len() const
    {
        return static_cast<int32>(this->length());
    }
    
    // Mid - returns substring
    FString Mid(int32 start, int32 count = INT32_MAX) const
    {
        if (start < 0 || start >= static_cast<int32>(this->length()))
            return FString();
        if (count == INT32_MAX)
            return FString(this->substr(start));
        return FString(this->substr(start, count));
    }
    
    // AppendChar - appends a character
    void AppendChar(char ch)
    {
        this->push_back(ch);
    }
    
    // Static Printf method for UE compatibility
    static FString Printf(const char* fmt, ...)
    {
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        #ifdef _WIN32
            vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);
        #else
            vsnprintf(buffer, sizeof(buffer), fmt, args);
        #endif
        va_end(args);
        return FString(buffer);
    }
};

// Text macro for string literals
#define TEXT(x) x

// Character types
using ANSICHAR = char;

// UTF8 conversion macro (no-op in standalone since we use char*)
#define UTF8_TO_TCHAR(x) (x)
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

// Array type with UE-compatible methods
template<typename T>
class TArray : public std::vector<T>
{
public:
    using std::vector<T>::vector;
    
    // UE-compatible methods
    int32 Num() const { return static_cast<int32>(this->size()); }
    void Add(const T& item) { this->push_back(item); }
    void Add(T&& item) { this->push_back(std::move(item)); }
    void Empty() { this->clear(); }
    void Reserve(int32 count) { this->reserve(count); }
    bool IsEmpty() const { return this->empty(); }
    T& Last() { return this->back(); }
    const T& Last() const { return this->back(); }
    void Pop() { this->pop_back(); }
    bool IsValidIndex(int32 index) const { return index >= 0 && index < Num(); }
    void Insert(const T& item, int32 index) { this->insert(this->begin() + index, item); }
    void Insert(T&& item, int32 index) { this->insert(this->begin() + index, std::move(item)); }
    
    // Additional methods for bytecode serialization
    T* GetData() { return this->data(); }
    const T* GetData() const { return this->data(); }
    void SetNum(int32 count) { this->resize(count); }
    void SetNumUninitialized(int32 count) { this->resize(count); }
    void Append(const TArray<T>& other) { this->insert(this->end(), other.begin(), other.end()); }
    void Append(const T* ptr, int32 count) { this->insert(this->end(), ptr, ptr + count); }
};

// Pair type (UE uses TPair)
template<typename K, typename V>
struct TPair
{
    K Key;
    V Value;
    
    TPair() = default;
    TPair(const K& k, const V& v) : Key(k), Value(v) {}
};

// Map type with UE-compatible methods
template<typename K, typename V>
class TMap : public std::map<K, V>
{
public:
    using std::map<K, V>::map;
    
    // UE-compatible methods
    void Add(const K& key, const V& value)
    {
        (*this)[key] = value;
    }
    
    V* Find(const K& key)
    {
        auto it = this->find(key);
        return (it != this->end()) ? &it->second : nullptr;
    }
    
    const V* Find(const K& key) const
    {
        auto it = this->find(key);
        return (it != this->end()) ? &it->second : nullptr;
    }
    
    bool Contains(const K& key) const
    {
        return this->find(key) != this->end();
    }
};

// Set type with UE-compatible methods
template<typename T>
class TSet : public std::set<T>
{
public:
    using std::set<T>::set;
    
    // UE-compatible methods
    void Add(const T& item)
    {
        this->insert(item);
    }
    
    bool Contains(const T& item) const
    {
        return this->find(item) != this->end();
    }
    
    void Empty()
    {
        this->clear();
    }
    
    int32 Num() const
    {
        return static_cast<int32>(this->size());
    }
};

// Shared pointer with UE-compatible methods
template<typename T>
class TSharedPtr : public std::shared_ptr<T>
{
public:
    using std::shared_ptr<T>::shared_ptr;
    
    // UE-compatible methods
    bool IsValid() const { return this->get() != nullptr; }
    T* Get() const { return this->get(); }
};

// Shared reference (non-nullable shared pointer)
template<typename T>
using TSharedRef = TSharedPtr<T>;

// Make shared
template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... args)
{
    TSharedPtr<T> ptr;
    static_cast<std::shared_ptr<T>&>(ptr) = std::make_shared<T>(std::forward<Args>(args)...);
    return ptr;
}

// Static cast for shared pointers
template<typename ToType, typename FromType>
TSharedPtr<ToType> StaticCastSharedPtr(const TSharedPtr<FromType>& ptr)
{
    TSharedPtr<ToType> result;
    static_cast<std::shared_ptr<ToType>&>(result) = std::static_pointer_cast<ToType>(ptr);
    return result;
}

// String utilities
namespace UE::String
{
    inline FString Printf(const char* fmt, ...)
    {
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        return FString(buffer);
    }
}

// FString static Printf method
namespace FStringImpl
{
    inline FString Printf(const char* fmt, ...)
    {
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        return FString(buffer);
    }
    
    // Forward declare ToLower for use in Find
    inline FString ToLower(const FString& str)
    {
        FString result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    inline int32 Find(const FString& str, const FString& search, ESearchCase searchCase = ESearchCase::CaseSensitive)
    {
        if (searchCase == ESearchCase::IgnoreCase)
        {
            FString lowerStr = ToLower(str);
            FString lowerSearch = ToLower(search);
            size_t pos = lowerStr.find(lowerSearch);
            return (pos != FString::npos) ? static_cast<int32>(pos) : -1;
        }
        size_t pos = str.find(search);
        return (pos != FString::npos) ? static_cast<int32>(pos) : -1;
    }
    
    inline FString Left(const FString& str, int32 count)
    {
        return str.substr(0, count);
    }
    
    inline FString Right(const FString& str, int32 count)
    {
        return str.substr(str.length() - count);
    }
    
    inline FString Mid(const FString& str, int32 start, int32 count = -1)
    {
        if (count < 0) return str.substr(start);
        return str.substr(start, count);
    }
    
    inline bool StartsWith(const FString& str, const FString& prefix)
    {
        return str.find(prefix) == 0;
    }
    
    inline bool EndsWith(const FString& str, const FString& suffix)
    {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    
    inline FString ToUpper(const FString& str)
    {
        FString result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
    
    inline void TrimStart(FString& str)
    {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }
    
    inline void TrimEnd(FString& str)
    {
        str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), str.end());
    }
    
    inline void Trim(FString& str)
    {
        TrimStart(str);
        TrimEnd(str);
    }
    
    inline bool IsEmpty(const FString& str)
    {
        return str.empty();
    }
    
    inline int32 Len(const FString& str)
    {
        return static_cast<int32>(str.length());
    }
    
    inline FString SanitizeFloat(double value)
    {
        std::ostringstream oss;
        oss << value;
        FString result = oss.str();
        
        // Remove trailing zeros after decimal point
        if (result.find('.') != FString::npos)
        {
            result.erase(result.find_last_not_of('0') + 1, FString::npos);
            if (result.back() == '.') result.pop_back();
        }
        
        return result;
    }
}

// Math utilities
namespace FMath
{
    inline bool IsNearlyEqual(double a, double b, double tolerance = 0.0001)
    {
        return std::abs(a - b) <= tolerance;
    }
}

// C String utilities (FCString)
namespace FCString
{
    inline int32 Atoi(const char* str)
    {
        return std::atoi(str);
    }
    
    inline double Atod(const char* str)
    {
        return std::atof(str);
    }
}

// Platform time utilities
namespace FPlatformTime
{
    inline double Seconds()
    {
        return static_cast<double>(std::clock()) / CLOCKS_PER_SEC;
    }
}

// Compression utilities (stub - no compression in standalone)
#define NAME_Zlib 0
namespace FCompression
{
    inline int32 CompressMemoryBound(int32 format, int32 uncompressedSize)
    {
        // Return a generous upper bound (original size + 10% + 256 bytes)
        return uncompressedSize + (uncompressedSize / 10) + 256;
    }
    
    inline bool CompressMemory(int32 format, void* compressedBuffer, int32& compressedSize, 
                               const void* uncompressedBuffer, int32 uncompressedSize)
    {
        // Stub: Just copy the data without compression
        if (compressedSize < uncompressedSize)
            return false;
        memcpy(compressedBuffer, uncompressedBuffer, uncompressedSize);
        compressedSize = uncompressedSize;
        return true;
    }
    
    inline bool UncompressMemory(int32 format, void* uncompressedBuffer, int32 uncompressedSize,
                                 const void* compressedBuffer, int32 compressedSize)
    {
        // Stub: Just copy the data (assuming no compression)
        if (uncompressedSize != compressedSize)
            return false;
        memcpy(uncompressedBuffer, compressedBuffer, compressedSize);
        return true;
    }
}

// Logging macros (standalone - output to console)
#define SCRIPT_LOG(msg) std::cout << "[LOG] " << msg << std::endl
#define SCRIPT_LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl
#define SCRIPT_LOG_WARNING(msg) std::cout << "[WARNING] " << msg << std::endl

// Generated body macro (no-op in standalone)
#define GENERATED_BODY()

// API export macro (no-op in standalone)
#define SCRIPTING_API

// Check macro
#define check(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "Check failed: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while(0)

#define checkf(expr, fmt, ...) \
    do { \
        if (!(expr)) { \
            std::cerr << "Check failed: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            fprintf(stderr, fmt, ##__VA_ARGS__); \
            std::cerr << std::endl; \
            std::abort(); \
        } \
    } while(0)

// File utilities
namespace FPlatformFile_Utils
{
    inline bool FileExists(const FString& path)
    {
        std::ifstream f(path);
        return f.good();
    }
    
    inline bool LoadFileToString(FString& result, const FString& path)
    {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        result = buffer.str();
        return true;
    }
    
    inline bool SaveStringToFile(const FString& data, const FString& path)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        
        file.write(data.c_str(), data.length());
        return file.good();
    }
    
    inline bool SaveArrayToFile(const TArray<uint8>& data, const FString& path)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return file.good();
    }
    
    inline bool LoadFileToArray(TArray<uint8>& result, const FString& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        result.resize(size);
        return file.read(reinterpret_cast<char*>(result.data()), size).good();
    }
    
    // Sanitize float string (remove trailing zeros)
    inline FString SanitizeFloat(const FString& str)
    {
        FString result = str;
        if (result.find('.') != FString::npos)
        {
            result.erase(result.find_last_not_of('0') + 1, FString::npos);
            if (!result.empty() && result.back() == '.')
                result.pop_back();
        }
        return result;
    }
}

// FPaths - Path utilities (UE-compatible API)
struct FPaths
{
    static FString GetBaseFilename(const FString& path, bool bRemoveExtension = true)
    {
        size_t lastSlash = path.find_last_of("/\\");
        FString filename = (lastSlash != FString::npos) ? FString(path.substr(lastSlash + 1)) : FString(path);
        
        if (bRemoveExtension)
        {
            size_t lastDot = filename.find_last_of('.');
            if (lastDot != FString::npos)
                filename = FString(filename.substr(0, lastDot));
        }
        return filename;
    }
    
    static FString GetCleanFilename(const FString& path)
    {
        size_t lastSlash = path.find_last_of("/\\");
        return (lastSlash != FString::npos) ? FString(path.substr(lastSlash + 1)) : FString(path);
    }
    
    static FString GetExtension(const FString& path)
    {
        size_t lastDot = path.find_last_of('.');
        return (lastDot != FString::npos) ? FString(path.substr(lastDot + 1)) : FString();
    }
    
    static FString GetPath(const FString& path)
    {
        size_t lastSlash = path.find_last_of("/\\");
        return (lastSlash != FString::npos) ? FString(path.substr(0, lastSlash)) : FString();
    }
    
    static FString ProjectDir()
    {
        // For standalone, return current directory
        char buffer[1024];
        #ifdef _WIN32
            GetCurrentDirectoryA(sizeof(buffer), buffer);
        #else
            getcwd(buffer, sizeof(buffer));
        #endif
        return FString(buffer);
    }
    
    static FString Combine(const FString& a, const FString& b)
    {
        if (a.empty()) return b;
        if (b.empty()) return a;
        char sep = '/';
        #ifdef _WIN32
            sep = '\\';
        #endif
        if (a.back() == '/' || a.back() == '\\')
            return a + b;
        return a + sep + b;
    }
    
    static FString NormalizeFilename(const FString& path)
    {
        FString result = path;
        // Replace backslashes with forward slashes
        std::replace(result.begin(), result.end(), '\\', '/');
        // Remove duplicate slashes
        size_t pos = 0;
        while ((pos = result.find("//", pos)) != FString::npos)
        {
            result.erase(pos, 1);
        }
        return result;
    }
    
    static bool FileExists(const FString& path)
    {
        return FPlatformFile_Utils::FileExists(path);
    }
};

// FFileHelper - File I/O utilities (UE-compatible API)
struct FFileHelper
{
    static bool LoadFileToString(FString& result, const FString& path)
    {
        return FPlatformFile_Utils::LoadFileToString(result, path);
    }
    
    static bool SaveStringToFile(const FString& data, const FString& path)
    {
        return FPlatformFile_Utils::SaveStringToFile(data, path);
    }
    
    static bool SaveArrayToFile(const TArray<uint8>& data, const FString& path)
    {
        return FPlatformFile_Utils::SaveArrayToFile(data, path);
    }
    
    static bool LoadFileToArray(TArray<uint8>& result, const FString& path)
    {
        return FPlatformFile_Utils::LoadFileToArray(result, path);
    }
};

// FString Printf (UE-compatible)
inline FString FStringPrintf(const char* fmt, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    #ifdef _WIN32
        vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);
    #else
        vsnprintf(buffer, sizeof(buffer), fmt, args);
    #endif
    va_end(args);
    return FString(buffer);
}

// =============================================================================
// DateTime
// =============================================================================

struct FDateTime
{
    time_t Value;
    
    FDateTime() : Value(time(nullptr)) {}
    FDateTime(time_t InValue) : Value(InValue) {}
    
    static FDateTime Now()
    {
        return FDateTime();
    }
    
    int64 GetTicks() const
    {
        return static_cast<int64>(Value);
    }
    
    FString ToString(const FString& Format = "%Y.%m.%d-%H.%M.%S") const
    {
        char buffer[128];
        struct tm timeinfo;
        #ifdef _WIN32
            localtime_s(&timeinfo, &Value);
        #else
            localtime_r(&Value, &timeinfo);
        #endif
        strftime(buffer, sizeof(buffer), Format.c_str(), &timeinfo);
        return FString(buffer);
    }
    
    bool operator>(const FDateTime& Other) const { return Value > Other.Value; }
    bool operator<(const FDateTime& Other) const { return Value < Other.Value; }
};

// SHA Hash types
struct FSHAHash
{
    uint8 Hash[20];
    
    FString ToString() const
    {
        char buffer[41];
        buffer[40] = '\0';
        for (int i = 0; i < 20; i++)
        {
            #ifdef _WIN32
                sprintf_s(&buffer[i * 2], 3, "%02x", Hash[i]);
            #else
                sprintf(&buffer[i * 2], "%02x", Hash[i]);
            #endif
        }
        return FString(buffer);
    }
};

namespace FSHA1
{
    inline void HashBuffer(const void* Data, uint64 DataSize, FSHAHash& OutHash)
    {
        // Stub: Simple XOR hash for standalone
        memset(&OutHash, 0, sizeof(OutHash));
        const uint8* bytes = static_cast<const uint8*>(Data);
        for (uint64 i = 0; i < DataSize; i++)
        {
            OutHash.Hash[i % 20] ^= bytes[i];
        }
    }
}

// =============================================================================
// Enum Flags Support
// =============================================================================

#define ENUM_CLASS_FLAGS(EnumType) \
    inline EnumType operator|(EnumType a, EnumType b) { \
        return static_cast<EnumType>(static_cast<uint32>(a) | static_cast<uint32>(b)); \
    } \
    inline EnumType operator&(EnumType a, EnumType b) { \
        return static_cast<EnumType>(static_cast<uint32>(a) & static_cast<uint32>(b)); \
    } \
    inline EnumType& operator|=(EnumType& a, EnumType b) { \
        return a = a | b; \
    } \
    inline EnumType& operator&=(EnumType& a, EnumType b) { \
        return a = a & b; \
    } \
    inline EnumType operator~(EnumType a) { \
        return static_cast<EnumType>(~static_cast<uint32>(a)); \
    }

template<typename Enum>
inline bool EnumHasAnyFlags(Enum Flags, Enum Contains)
{
    return (static_cast<uint32>(Flags) & static_cast<uint32>(Contains)) != 0;
}

template<typename Enum>
inline bool EnumHasAllFlags(Enum Flags, Enum Contains)
{
    return (static_cast<uint32>(Flags) & static_cast<uint32>(Contains)) == static_cast<uint32>(Contains);
}

// =============================================================================
// MD5 Hash
// =============================================================================

struct FMD5
{
    static FString HashAnsiString(const FString& String)
    {
        // Simple hash for standalone (use proper MD5 in production)
        std::hash<std::string> hasher;
        size_t hash = hasher(String);
        
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%016llx%016llx", 
                 (unsigned long long)hash, (unsigned long long)(hash ^ 0xDEADBEEF));
        return FString(buffer);
    }
};

// =============================================================================
// Platform Info
// =============================================================================

struct FPlatformMisc
{
    static FString GetMachineName()
    {
        #ifdef _WIN32
            char buffer[256];
            DWORD size = sizeof(buffer);
            if (GetComputerNameA(buffer, &size))
                return FString(buffer);
        #endif
        return "Unknown";
    }
    
    static FString GetLoginName()
    {
        #ifdef _WIN32
            char buffer[256];
            DWORD size = sizeof(buffer);
            if (GetUserNameA(buffer, &size))
                return FString(buffer);
        #elif defined(__unix__) || defined(__APPLE__)
            const char* user = getenv("USER");
            if (user) return FString(user);
        #endif
        return "Unknown";
    }
};

// Compatibility wrapper for FPlatformProcess
struct FPlatformProcess
{
    static FString ComputerName() { return FPlatformMisc::GetMachineName(); }
    static FString UserName() { return FPlatformMisc::GetLoginName(); }
};

// =============================================================================
// Engine Version
// =============================================================================

#define ENGINE_MAJOR_VERSION 5
#define ENGINE_MINOR_VERSION 6

// =============================================================================
// Platform Macros
// =============================================================================

#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
    #define TEXT(x) x
#else
    #define PLATFORM_WINDOWS 0
    #define TEXT(x) x
#endif

#ifdef __APPLE__
    #define PLATFORM_MAC 1
#else
    #define PLATFORM_MAC 0
#endif

#ifdef __linux__
    #define PLATFORM_LINUX 1
#else
    #define PLATFORM_LINUX 0
#endif

// =============================================================================
// UTF-8 Conversion
// =============================================================================

class FTCHARToUTF8
{
public:
    FTCHARToUTF8(const FString& InString) : Utf8String(InString) {}
    
    const char* Get() const { return Utf8String.c_str(); }
    int32 Length() const { return static_cast<int32>(Utf8String.length()); }
    
private:
    FString Utf8String;
};

// Forward declarations
class FScriptVM;

// Native function type
using FNativeFunction = std::function<void(FScriptVM*)>;

#else

// Unreal Engine mode: Use real UE types
#include "CoreMinimal.h"

// Logging uses UE's system
// SCRIPT_LOG, etc. defined in ScriptLogger.h

#endif // STANDALONE_BUILD

