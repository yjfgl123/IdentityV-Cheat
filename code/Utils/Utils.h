#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cmath>


namespace Utils {
    std::string read_file_to_string(const std::string& file_path);
    FILE* auto_fopen(const std::string& filepath, const char* mode);
    uint64_t get_current_ms();
    std::string get_device_id();
    void encrypt_bytes(std::byte* data, size_t len);
    void decrypt_bytes(std::byte* data, size_t len);
    std::string generate_random_string(int length);
    std::wstring generate_random_wstring(int length);
    void* get_proc_address(void* handle, const char* function_name);
    void* get_module_entry_point(void* handle);
    float calc_distance(float x1, float y1, float z1, float x2, float y2, float z2);
    float calc_distance2d(float x1, float y1, float x2, float y2);
    bool is_key_pressed(char key);
}