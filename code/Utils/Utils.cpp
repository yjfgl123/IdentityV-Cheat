#include <windows.h>
#include <fstream>
#include <filesystem> 
#include <sstream> 
#include "Utils.h"
#include <iphlpapi.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <random>
#include <thread>

#pragma comment(lib, "iphlpapi.lib")

std::string GetMacAddress() {
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        return "";
    }
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            return "";
        }
    }
    std::string macAddress;
    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            if (pAdapter->Type == MIB_IF_TYPE_ETHERNET &&
                pAdapter->AddressLength > 0) {
                std::stringstream ss;
                for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                    if (i > 0) ss << ":";
                    ss << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(pAdapter->Address[i]);
                }
                macAddress = ss.str();
                break;
            }
            pAdapter = pAdapter->Next;
        }
    }
    free(pAdapterInfo);
    return macAddress;
}



std::string GetDiskSerialNumber() {
    std::string serialNumber;
    char volumeName[MAX_PATH + 1] = { 0 };
    char fileSystemName[MAX_PATH + 1] = { 0 };
    DWORD serialNumberValue = 0;
    DWORD maxComponentLength = 0;
    DWORD fileSystemFlags = 0;

    if (GetVolumeInformationA("C:\\",
        volumeName,
        ARRAYSIZE(volumeName),
        &serialNumberValue,
        &maxComponentLength,
        &fileSystemFlags,
        fileSystemName,
        ARRAYSIZE(fileSystemName))) {
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << serialNumberValue;
        serialNumber = ss.str();
    }
    return serialNumber;
}


std::string GetDeviceUniqueId() {
    return GetDiskSerialNumber() + "-" + GetMacAddress();
}

unsigned int DJB2Hash(const std::string& str) {
    unsigned int hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

unsigned int FNV1aHash(const std::string& str) {
    const unsigned int prime = 0x01000193; // 16777619
    unsigned int hash = 0x811C9DC5;        // 2166136261

    for (char c : str) {
        hash ^= static_cast<unsigned char>(c);
        hash *= prime;
    }
    return hash;
}

std::string GetHashedDeviceId() {
    std::string uniqueId = GetDeviceUniqueId();

    unsigned int hash1 = DJB2Hash(uniqueId);
    unsigned int hash2 = FNV1aHash(uniqueId);

    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << hash1
        << std::hex << std::setw(8) << std::setfill('0') << hash2;

    return ss.str();
}


namespace Utils {
    std::string read_file_to_string(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + file_path);
        }

        auto file_size = std::filesystem::file_size(file_path);

        std::string content(file_size, '\0');
        file.read(content.data(), file_size);
        return content;
    }

    FILE* auto_fopen(const std::string& filepath, const char* mode) {
        std::string dump_path = "dump_pyc\\" + filepath;
        std::filesystem::create_directories(std::filesystem::path(dump_path).parent_path());
        return fopen(dump_path.c_str(), mode);
    }
    uint64_t Utils::get_current_ms()
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return ms;
    }
    std::string get_device_id()
    {
        return GetHashedDeviceId();
    }
    constexpr uint32_t ENCRYPTION_SEED = 0xA5B357; // 可替换为动态密钥
    constexpr int NUM_ROUNDS = 16; // 增加轮数以提高安全性

    // 辅助函数：循环左移
    constexpr uint8_t rotl8(uint8_t x, uint8_t shift) {
        shift &= 7;
        return (x << shift) | (x >> (8 - shift));
    }

    // 辅助函数：循环右移
    constexpr uint8_t rotr8(uint8_t x, uint8_t shift) {
        shift &= 7;
        return (x >> shift) | (x << (8 - shift));
    }

    void encrypt_bytes(std::byte* data, size_t len) {
        if (len == 0) return;

        std::mt19937_64 rng(ENCRYPTION_SEED); // 使用64位MT以提高随机性
        std::uniform_int_distribution<size_t> dist(0, len - 1);

        for (int round = 0; round < NUM_ROUNDS; ++round) {
            // 生成本轮密钥流
            std::vector<uint8_t> keystream(len);
            for (size_t i = 0; i < len; ++i) {
                keystream[i] = rng() & 0xFF;
            }

            // 字节替换和混淆
            for (size_t i = 0; i < len; ++i) {
                uint8_t val = std::to_integer<uint8_t>(data[i]);
                uint8_t key = keystream[i];

                // 多层变换
                val ^= key;
                val = rotl8(val, 3);
                val += key;
                val = rotr8(val, 5);
                val ^= 0xAA; // 添加常量混淆

                data[i] = std::byte(val);
            }

            // 字节置换：Fisher-Yates洗牌算法变种
            for (size_t i = len - 1; i > 0; --i) {
                size_t j = dist(rng) % (i + 1);
                std::swap(data[i], data[j]);
            }

            // 扩散操作：让每个字节影响后续字节
            for (size_t i = 1; i < len; ++i) {
                uint8_t prev = std::to_integer<uint8_t>(data[i - 1]);
                uint8_t curr = std::to_integer<uint8_t>(data[i]);
                data[i] = std::byte(curr ^ (prev >> 3));
            }
        }
    }

    void decrypt_bytes(std::byte* data, size_t len) {
        if (len == 0) return;

        std::mt19937_64 rng(ENCRYPTION_SEED);

        // 预生成所有轮次的随机状态
        std::vector<std::vector<uint8_t>> round_keys(NUM_ROUNDS);
        std::vector<std::vector<size_t>> round_permutations(NUM_ROUNDS);

        for (int round = 0; round < NUM_ROUNDS; ++round) {
            // 生成密钥流
            round_keys[round].resize(len);
            for (size_t i = 0; i < len; ++i) {
                round_keys[round][i] = rng() & 0xFF;
            }

            // 生成置换序列
            std::uniform_int_distribution<size_t> dist(0, len - 1);
            round_permutations[round].resize(len);
            for (size_t i = 0; i < len; ++i) {
                round_permutations[round][i] = i;
            }

            // 生成与加密时相同的随机置换
            for (size_t i = len - 1; i > 0; --i) {
                size_t j = dist(rng) % (i + 1);
                std::swap(round_permutations[round][i], round_permutations[round][j]);
            }
        }

        // 反向处理所有轮次
        for (int round = NUM_ROUNDS - 1; round >= 0; --round) {
            // 反向扩散操作
            for (size_t i = len - 1; i > 0; --i) {
                uint8_t prev = std::to_integer<uint8_t>(data[i - 1]);
                uint8_t curr = std::to_integer<uint8_t>(data[i]);
                data[i] = std::byte(curr ^ (prev >> 3));
            }

            // 反向置换
            std::vector<std::byte> temp(len);
            for (size_t i = 0; i < len; ++i) {
                temp[round_permutations[round][i]] = data[i];
            }
            std::copy(temp.begin(), temp.end(), data);

            // 反向字节变换
            for (size_t i = 0; i < len; ++i) {
                uint8_t val = std::to_integer<uint8_t>(data[i]);
                uint8_t key = round_keys[round][i];

                val ^= 0xAA; // 反向常量混淆
                val = rotl8(val, 5); // 反向rotr5
                val -= key;
                val = rotr8(val, 3); // 反向rotl3
                val ^= key;

                data[i] = std::byte(val);
            }
        }
    }
    std::string generate_random_string(int length)
    {
        const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, charset.size() - 1);

        std::string result;
        result.reserve(length);

        for (int i = 0; i < length; ++i) {
            result += charset[dis(gen)];
        }
        return result;
    }

    std::wstring generate_random_wstring(int length)
    {
        const std::wstring charset = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, charset.size() - 1);

        std::wstring result;
        result.reserve(length);

        for (int i = 0; i < length; ++i) {
            result += charset[dis(gen)];
        }
        return result;
    }

    void* get_proc_address(void* handle, const char* function_name)
    {
        const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)handle;
        const auto lpImageNTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
        if (lpImageNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0)
            return nullptr;

        const auto lpImageExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD_PTR)handle + lpImageNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
        const DWORD_PTR dNumberOfNames = lpImageExportDirectory->NumberOfNames;

        for (int i = 0; i < (int)dNumberOfNames; i++)
        {
            const auto lpCurrentFunctionName = (LPSTR)(((DWORD*)(lpImageExportDirectory->AddressOfNames + (DWORD_PTR)handle))[i] + (DWORD_PTR)handle);
            const auto lpCurrentOridnal = ((WORD*)(lpImageExportDirectory->AddressOfNameOrdinals + (DWORD_PTR)handle))[i];
            const auto addRVA = ((DWORD*)((DWORD_PTR)handle + lpImageExportDirectory->AddressOfFunctions))[lpCurrentOridnal];
            if (strcmp(lpCurrentFunctionName, function_name) == 0)
                return (LPVOID)((DWORD_PTR)handle + addRVA);
        }
        return nullptr;
    }
    void* get_module_entry_point(void* handle)
    {
        const auto lpImageDOSHeader = (PIMAGE_DOS_HEADER)handle;
        const auto lpImageNTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)lpImageDOSHeader + lpImageDOSHeader->e_lfanew);
        return (void*)((DWORD_PTR)handle + lpImageNTHeader->OptionalHeader.AddressOfEntryPoint);
    }
    float calc_distance(float x1, float y1, float z1, float x2, float y2, float z2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    float calc_distance2d(float x1, float y1, float x2, float y2)
    {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }

    bool is_key_pressed(char key) {
        int vk;
        switch (key) {
        case ' ': vk = VK_SPACE; break;     // 空格键
        case '\t': vk = VK_TAB; break;      // Tab键
        case '\r': vk = VK_RETURN; break;   // 回车键
        case '\b': vk = VK_BACK; break;     // 退格键
        case 27: vk = VK_ESCAPE; break;     // ESC键 (ASCII 27)
        default:
            vk = VkKeyScanA(key) & 0xFF;
            break;
        }

        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }
}