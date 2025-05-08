#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>
#include <set>
#include <unordered_map>
#include <stdexcept>
#include <optional>
#include <algorithm>
#include <cstdlib>
#include <future>
#include <mutex>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

std::set<std::string> modified_files;
std::mutex g_modified_mutex;


std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string bytesToHex(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    oss << std::hex;
    for (unsigned char b : data) {
        oss.width(2);
        oss.fill('0');
        oss << static_cast<int>(b);
    }
    return oss.str();
}

std::vector<unsigned char> hexStringToBytes(const std::string &hex) {
    std::vector<unsigned char> bytes;
    if(hex.size() % 2 != 0)
        throw std::invalid_argument("Hex string length must be even.");
    for (size_t i = 0; i < hex.size(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<unsigned char> decimalTo4Byte(int value) {
    std::vector<unsigned char> bytes(4);
    for (int i = 0; i < 4; i++) {
        bytes[i] = static_cast<unsigned char>((value >> (8 * i)) & 0xFF);
    }
    return bytes;
}

std::string intToHexLittleEndian(int n) {
    auto bytes = decimalTo4Byte(n);
    return bytesToHex(bytes);
}


size_t findSubvector(const std::vector<unsigned char>& data,
                     const std::vector<unsigned char>& pattern,
                     size_t start) {
    if (pattern.empty() || data.size() < pattern.size())
        return std::string::npos;
    for (size_t i = start; i <= data.size() - pattern.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < pattern.size(); j++) {
            if (data[i+j] != pattern[j]) { found = false; break; }
        }
        if (found)
            return i;
    }
    return std::string::npos;
}



struct Markers {
    std::vector<unsigned char> start_marker;
    std::vector<unsigned char> end_marker;
    std::vector<unsigned char> target_marker;
};


Markers getMarkers(const std::string &start_marker_input,
                   const std::string &end_marker_input,
                   int target_marker_decimal) {
    Markers m;
    m.start_marker = hexStringToBytes(start_marker_input);
    m.end_marker   = hexStringToBytes(end_marker_input);
    m.target_marker = decimalTo4Byte(target_marker_decimal);
    return m;
}



struct FoundBlock {
    std::string file;
    std::string first_target_value;
    size_t first_target_position;
    std::string second_target_value;
    size_t second_target_position;
};


std::optional<std::vector<unsigned char>> little_endian_append_00(const std::vector<unsigned char>& byte_data) {
    if (byte_data.size() < 4) return std::nullopt;
    uint32_t decimal_value = byte_data[0] | (byte_data[1] << 8) | (byte_data[2] << 16) | (byte_data[3] << 24);
    std::string dec_str = std::to_string(decimal_value) + "00";
    try {
        uint32_t new_decimal_value = std::stoul(dec_str);
        if (new_decimal_value > 0xFFFFFFFF)
            return std::nullopt;
        std::vector<unsigned char> new_bytes(4);
        new_bytes[0] = new_decimal_value & 0xFF;
        new_bytes[1] = (new_decimal_value >> 8) & 0xFF;
        new_bytes[2] = (new_decimal_value >> 16) & 0xFF;
        new_bytes[3] = (new_decimal_value >> 24) & 0xFF;
        return new_bytes;
    } catch (...) {
        return std::nullopt;
    }
}


std::optional<std::vector<unsigned char>> little_endian_remove_00(const std::vector<unsigned char>& byte_data) {
    if (byte_data.size() < 4) return std::nullopt;
    uint32_t decimal_value = byte_data[0] | (byte_data[1] << 8) | (byte_data[2] << 16) | (byte_data[3] << 24);
    uint32_t new_decimal_value = decimal_value / 100;
    if (new_decimal_value > 0xFFFFFFFF)
        return std::nullopt;
    std::vector<unsigned char> new_bytes(4);
    new_bytes[0] = new_decimal_value & 0xFF;
    new_bytes[1] = (new_decimal_value >> 8) & 0xFF;
    new_bytes[2] = (new_decimal_value >> 16) & 0xFF;
    new_bytes[3] = (new_decimal_value >> 24) & 0xFF;
    return new_bytes;
}


void findBlocksInFile(const std::vector<unsigned char>& content,
                      const std::string &file_path,
                      std::vector<FoundBlock> &found_blocks,
                      const std::vector<unsigned char> &start_marker,
                      const std::vector<unsigned char> &end_marker,
                      const std::vector<unsigned char> &target_marker) {
    size_t idx_start = 0;
    while (idx_start < content.size()) {
        size_t start_idx = findSubvector(content, start_marker, idx_start);
        if (start_idx == std::string::npos) break;
        size_t end_idx = findSubvector(content, end_marker, start_idx + start_marker.size());
        if (end_idx == std::string::npos) break;
        if (end_idx - (start_idx + start_marker.size()) == 14) {
            size_t target_start_idx = end_idx + end_marker.size() + 15;
            if (target_start_idx + 4 > content.size()) break;
            std::vector<unsigned char> first_target_value(content.begin() + target_start_idx,
                                                           content.begin() + target_start_idx + 4);
            size_t block_start = target_start_idx + 4;
            size_t second_target_idx = findSubvector(content, first_target_value, block_start);
            if (second_target_idx != std::string::npos) {
                FoundBlock block;
                block.file = file_path;
                block.first_target_value = bytesToHex(first_target_value);
                block.first_target_position = target_start_idx;
                block.second_target_value = block.first_target_value;
                block.second_target_position = second_target_idx;
                found_blocks.push_back(block);
            }
        }
        idx_start = end_idx + end_marker.size();
    }
}


void findBlocksInFileVehicle(const std::vector<unsigned char>& content,
                             const std::string &file_path,
                             std::vector<FoundBlock> &found_blocks,
                             const std::vector<unsigned char> &start_marker,
                             const std::vector<unsigned char> &end_marker,
                             const std::vector<unsigned char> &target_marker) {
    size_t idx_start = 0;
    while (idx_start < content.size()) {
        size_t start_idx = findSubvector(content, start_marker, idx_start);
        if (start_idx == std::string::npos) break;
        size_t end_idx = findSubvector(content, end_marker, start_idx + start_marker.size());
        if (end_idx == std::string::npos) break;
        if (end_idx - (start_idx + start_marker.size()) == 14) {
            size_t target_start_idx = end_idx + end_marker.size() + 15;
            if (target_start_idx + 4 > content.size()) break;
            std::vector<unsigned char> first_target_value(content.begin() + target_start_idx,
                                                           content.begin() + target_start_idx + 4);
            size_t block_start = target_start_idx + 4;
            auto second_target_opt = little_endian_append_00(first_target_value);
            if (!second_target_opt.has_value()) {
                idx_start = end_idx + end_marker.size();
                continue;
            }
            auto second_target_value = second_target_opt.value();
            size_t second_target_idx = findSubvector(content, second_target_value, block_start);
            if (second_target_idx != std::string::npos) {
                FoundBlock block;
                block.file = file_path;
                block.first_target_value = bytesToHex(first_target_value);
                block.first_target_position = target_start_idx;
                block.second_target_value = bytesToHex(second_target_value);
                block.second_target_position = second_target_idx;
                found_blocks.push_back(block);
            }
        }
        idx_start = end_idx + end_marker.size();
    }
}

// ========== 文件处理 ==========

void processFile(const std::string &file_path,
                 const std::vector<unsigned char> &start_marker,
                 const std::vector<unsigned char> &end_marker,
                 const std::vector<unsigned char> &target_marker,
                 std::vector<FoundBlock> &found_blocks,
                 std::vector<FoundBlock> &found_blocks_no_symmetric) {
    try {
        std::ifstream ifs(file_path, std::ios::binary);
        if (!ifs) {
            std::cerr << "Error reading file " << file_path << "\n";
            return;
        }
        std::vector<unsigned char> content((std::istreambuf_iterator<char>(ifs)),
                                             std::istreambuf_iterator<char>());
        ifs.close();
        findBlocksInFile(content, file_path, found_blocks, start_marker, end_marker, target_marker);
        findBlocksInFileVehicle(content, file_path, found_blocks_no_symmetric, start_marker, end_marker, target_marker);
    } catch (const std::exception &e) {
        std::cerr << "Error reading file " << file_path << ": " << e.what() << "\n";
    }
}

// 并行遍历指定文件夹中的所有文件，查找块
void findHexBlocksInFolder(const std::string &folder_path,
                           const std::vector<unsigned char> &start_marker,
                           const std::vector<unsigned char> &end_marker,
                           const std::vector<unsigned char> &target_marker,
                           std::vector<FoundBlock> &found_blocks,
                           std::vector<FoundBlock> &found_blocks_no_symmetric) {
    std::vector<std::string> files;
    for (auto &entry : fs::recursive_directory_iterator(folder_path)) {
        if (fs::is_regular_file(entry.path()))
            files.push_back(entry.path().string());
    }
    std::vector<std::future<void>> futures;
    std::mutex mtx;
    for (const auto &file : files) {
        futures.push_back(std::async(std::launch::async, [&, file]() {
            std::vector<FoundBlock> local_found;
            std::vector<FoundBlock> local_found_no_sym;
            processFile(file, start_marker, end_marker, target_marker, local_found, local_found_no_sym);
            std::lock_guard<std::mutex> lock(mtx);
            found_blocks.insert(found_blocks.end(), local_found.begin(), local_found.end());
            found_blocks_no_symmetric.insert(found_blocks_no_symmetric.end(), local_found_no_sym.begin(), local_found_no_sym.end());
        }));
    }
    for (auto &fut : futures) {
        fut.get();
    }
}

// ========== YAML 配置解析 ==========

struct YAMLConfig {
    std::vector<std::pair<int, int>> swap_pairs;
    std::string start_marker;
    std::string end_marker;
};

YAMLConfig readyaml(const std::string &file_path) {
    YAMLConfig config;
    std::ifstream ifs(file_path);
    if (!ifs) {
        std::cerr << "无法打开 YAML 文件 " << file_path << "\n";
        return config;
    }
    std::string line;
    bool in_swap_pairs = false;
    bool in_hex_markers = false;
    std::regex pair_regex("-\\s*\\[\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\]");
    std::regex key_value_regex("^\\s*(\\w+)\\s*:\\s*\"?(.*?)\"?$");
    while (std::getline(ifs, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.find("swap_pairs:") == 0) {
            in_swap_pairs = true;
            in_hex_markers = false;
            continue;
        }
        if (line.find("hex_markers:") == 0) {
            in_hex_markers = true;
            in_swap_pairs = false;
            continue;
        }
        if (in_swap_pairs && line[0] == '-') {
            std::smatch m;
            if (std::regex_search(line, m, pair_regex)) {
                int a = std::stoi(m[1].str());
                int b = std::stoi(m[2].str());
                config.swap_pairs.push_back({a, b});
            }
        }
        if (in_hex_markers) {
            std::smatch m;
            if (std::regex_search(line, m, key_value_regex)) {
                std::string key = m[1].str();
                std::string value = m[2].str();
                if (key == "start")
                    config.start_marker = value;
                else if (key == "end")
                    config.end_marker = value;
            }
        }
    }
    return config;
}


void swap_hex_values_in_file(const std::string &file1, size_t pos1, size_t pos2,
                              const std::string &file2, size_t pos3, size_t pos4,
                              const std::string &value1, const std::string &value2) {
    if (!fs::exists(file1) || !fs::exists(file2)) {
        std::cerr << "错误: 文件 " << file1 << " 或 " << file2 << " 不存在！\n";
        return;
    }
    std::vector<unsigned char> content1, content2;
    {
        std::ifstream ifs(file1, std::ios::binary);
        content1 = std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)),
                                                std::istreambuf_iterator<char>());
    }
    {
        std::ifstream ifs(file2, std::ios::binary);
        content2 = std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)),
                                                std::istreambuf_iterator<char>());
    }
    auto bytes_value1 = hexStringToBytes(value1);
    auto bytes_value2 = hexStringToBytes(value2);
    bool modified = false;
    if (file1 == file2) {
        if (std::equal(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos1) &&
            std::equal(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos2)) {
            std::copy(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos1);
            std::copy(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos2);
            modified = true;
        }
        if (std::equal(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos3) &&
            std::equal(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos4)) {
            std::copy(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos3);
            std::copy(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos4);
            modified = true;
        }
    } else {
        if (std::equal(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos1) &&
            std::equal(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos2)) {
            std::copy(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos1);
            std::copy(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos2);
            modified = true;
        }
        if (std::equal(bytes_value2.begin(), bytes_value2.end(), content2.begin() + pos3) &&
            std::equal(bytes_value2.begin(), bytes_value2.end(), content2.begin() + pos4)) {
            std::copy(bytes_value1.begin(), bytes_value1.end(), content2.begin() + pos3);
            std::copy(bytes_value1.begin(), bytes_value1.end(), content2.begin() + pos4);
            modified = true;
        }
    }
    if (modified) {
        if (file1 == file2) {
            std::ofstream ofs(file1, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(content1.data()), content1.size());
        } else {
            std::ofstream ofs(file1, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(content1.data()), content1.size());
            std::ofstream ofs2(file2, std::ios::binary);
            ofs2.write(reinterpret_cast<const char*>(content2.data()), content2.size());
        }
        {
            std::lock_guard<std::mutex> lock(g_modified_mutex);
            modified_files.insert(file1);
            modified_files.insert(file2);
        }
    }
}

// 交换文件中指定位置的 16 进制数据（载具用）
void swap_hex_values_in_file_vehicle(const std::string &file1, size_t pos1, size_t pos2,
                              const std::string &file2, size_t pos3, size_t pos4,
                              const std::string &value1, const std::string &value2) {
    if (!fs::exists(file1) || !fs::exists(file2)) {
        std::cerr << "错误: 文件 " << file1 << " 或 " << file2 << " 不存在！\n";
        return;
    }
    std::vector<unsigned char> content1, content2;
    {
        std::ifstream ifs(file1, std::ios::binary);
        content1 = std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)),
                                                std::istreambuf_iterator<char>());
    }
    {
        std::ifstream ifs(file2, std::ios::binary);
        content2 = std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)),
                                                std::istreambuf_iterator<char>());
    }
    auto bytes_value1 = hexStringToBytes(value1);
    auto bytes_value2 = hexStringToBytes(value2);
    bool modified = false;
    if (file1 == file2) {
        if (std::equal(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos2)) {
            std::copy(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos2);
            modified = true;
        }
        if (std::equal(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos4)) {
            std::copy(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos4);
            modified = true;
        }
    } else {
        if (std::equal(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos2)) {
            std::copy(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos2);
            modified = true;
        }
        if (std::equal(bytes_value2.begin(), bytes_value2.end(), content2.begin() + pos4)) {
            std::copy(bytes_value1.begin(), bytes_value1.end(), content2.begin() + pos4);
            modified = true;
        }
    }
    if (modified) {
        if (file1 == file2) {
            std::ofstream ofs(file1, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(content1.data()), content1.size());
        } else {
            std::ofstream ofs(file1, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(content1.data()), content1.size());
            std::ofstream ofs2(file2, std::ios::binary);
            ofs2.write(reinterpret_cast<const char*>(content2.data()), content2.size());
        }
        {
            std::lock_guard<std::mutex> lock(g_modified_mutex);
            modified_files.insert(file1);
            modified_files.insert(file2);
        }
    }
}

// 交换文件中指定位置的 16 进制数据（武器用）
void swap_hex_values_in_file_weapon(const std::string &file1, size_t pos1, size_t pos2,
                              const std::string &file2, size_t pos3, size_t pos4,
                              const std::string &value1, const std::string &value2) {
    if (!fs::exists(file1) || !fs::exists(file2)) {
        std::cerr << "错误: 文件 " << file1 << " 或 " << file2 << " 不存在！\n";
        return;
    }
    std::vector<unsigned char> content1, content2;
    {
        std::ifstream ifs(file1, std::ios::binary);
        content1 = std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)),
                                                std::istreambuf_iterator<char>());
    }
    {
        std::ifstream ifs(file2, std::ios::binary);
        content2 = std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)),
                                                std::istreambuf_iterator<char>());
    }
    auto bytes_value1 = hexStringToBytes(value1);
    auto bytes_value2 = hexStringToBytes(value2);
    bool modified = false;
    if (file1 == file2) {
        if (std::equal(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos2)) {
            std::copy(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos2);
            modified = true;
        }
        if ( std::equal(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos3) &&
             std::equal(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos4) ) {
            std::copy(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos3);
            std::copy(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos4);
            modified = true;
        }
    } else {
        if (std::equal(bytes_value1.begin(), bytes_value1.end(), content1.begin() + pos2)) {
            std::copy(bytes_value2.begin(), bytes_value2.end(), content1.begin() + pos2);
            modified = true;
        }
        if ( std::equal(bytes_value2.begin(), bytes_value2.end(), content2.begin() + pos3) &&
             std::equal(bytes_value2.begin(), bytes_value2.end(), content2.begin() + pos4) ) {
            std::copy(bytes_value1.begin(), bytes_value1.end(), content2.begin() + pos3);
            std::copy(bytes_value1.begin(), bytes_value1.end(), content2.begin() + pos4);
            modified = true;
        }
    }
    if (modified) {
        if (file1 == file2) {
            std::ofstream ofs(file1, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(content1.data()), content1.size());
        } else {
            std::ofstream ofs(file1, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(content1.data()), content1.size());
            std::ofstream ofs2(file2, std::ios::binary);
            ofs2.write(reinterpret_cast<const char*>(content2.data()), content2.size());
        }
        {
            std::lock_guard<std::mutex> lock(g_modified_mutex);
            modified_files.insert(file1);
            modified_files.insert(file2);
        }
    }
}


void copy_uexp_files(const std::string &source_folder, const std::string &destination_folder) {
    try {
        if (fs::exists(destination_folder))
            fs::remove_all(destination_folder);
        fs::create_directories(destination_folder);
        for (auto &entry : fs::directory_iterator(source_folder)) {
            std::string src = entry.path().string();
            std::string dest = (fs::path(destination_folder) / entry.path().filename()).string();
            if (fs::is_regular_file(entry.path()))
                fs::copy_file(entry.path(), dest, fs::copy_options::overwrite_existing);
            else if (fs::is_directory(entry.path()))
                fs::copy(entry.path(), dest, fs::copy_options::recursive);
        }
    } catch (const std::exception &e) {
        std::cerr << "发生错误：" << e.what() << "\n";
    }
}

// 删除指定目录中不在 modified_files 集合内的文件
void delete_unmodified_files(const std::string &directory, const std::set<std::string> &modified_files) {
    for (auto &entry : fs::directory_iterator(directory)) {
        std::string file_path = entry.path().string();
        if (modified_files.find(file_path) == modified_files.end() && fs::is_regular_file(entry.path()))
            fs::remove(entry.path());
    }
    std::cout << "美化完成，接下来请使用，uexp打包\n";
}



struct SwapPair {
    int first;
    int second;
};

int main() {

    copy_uexp_files("解包数据/uexp", "打包/uexp");

    std::string start_marker_input = "";
    std::string end_marker_input = "";
    int target_marker_decimal = 403211;
    std::string file_path = "cloth.yaml";
    std::string file_path_vehicle = "vehicle.yaml";
    std::string file_path_weapon = "weapon.yaml";

    // 3. 解析 YAML 文件（衣服、载具、武器）
    YAMLConfig cloth_config = readyaml(file_path);
    YAMLConfig vehicle_config = readyaml(file_path_vehicle);
    YAMLConfig weapon_config = readyaml(file_path_weapon);
    std::vector<SwapPair> cloth_to_swap;
    std::vector<SwapPair> vehicle_to_swap;
    std::vector<SwapPair> weapon_to_swap;
    for (const auto &p : cloth_config.swap_pairs)
        cloth_to_swap.push_back({p.first, p.second});
    for (const auto &p : vehicle_config.swap_pairs)
        vehicle_to_swap.push_back({p.first, p.second});
    for (const auto &p : weapon_config.swap_pairs)
        weapon_to_swap.push_back({p.first, p.second});
    if (!cloth_config.start_marker.empty()) start_marker_input = cloth_config.start_marker;
    if (!cloth_config.end_marker.empty()) end_marker_input = cloth_config.end_marker;

    {
        std::lock_guard<std::mutex> lock(g_modified_mutex);
        modified_files.clear();
    }


    Markers markers = getMarkers(start_marker_input, end_marker_input, target_marker_decimal);

    std::vector<FoundBlock> found_blocks, found_blocks_no_symmetric;
    findHexBlocksInFolder("打包/uexp", markers.start_marker, markers.end_marker, markers.target_marker,
                            found_blocks, found_blocks_no_symmetric);

    std::unordered_map<std::string, FoundBlock> found_blocks_dict;
    for (const auto &block : found_blocks)
        found_blocks_dict[block.first_target_value] = block;
    std::unordered_map<std::string, FoundBlock> found_blocks_no_symmetric_dict;
    for (const auto &block : found_blocks_no_symmetric)
        found_blocks_no_symmetric_dict[block.second_target_value] = block;

    std::vector<std::pair<FoundBlock, FoundBlock>> cloth_to_swap_temp;
    for (const auto &pair : cloth_to_swap) {
        std::string first_hex = intToHexLittleEndian(pair.first);
        std::string last_hex = intToHexLittleEndian(pair.second);
        if (found_blocks_dict.find(first_hex) != found_blocks_dict.end() &&
            found_blocks_dict.find(last_hex) != found_blocks_dict.end()) {
            cloth_to_swap_temp.push_back({found_blocks_dict[first_hex], found_blocks_dict[last_hex]});
        }
    }
    for (const auto &p : cloth_to_swap_temp) {
        swap_hex_values_in_file(p.first.file,
                                 p.first.first_target_position, p.first.second_target_position,
                                 p.second.file,
                                 p.second.first_target_position, p.second.second_target_position,
                                 p.first.first_target_value, p.second.first_target_value);
    }

    std::vector<std::pair<FoundBlock, FoundBlock>> vehicle_to_swap_temp;
    for (const auto &pair : vehicle_to_swap) {
        std::string first_hex = intToHexLittleEndian(pair.first);
        std::string last_hex = intToHexLittleEndian(pair.second);
        if (found_blocks_no_symmetric_dict.find(first_hex) != found_blocks_no_symmetric_dict.end() &&
            found_blocks_no_symmetric_dict.find(last_hex) != found_blocks_no_symmetric_dict.end()) {
            vehicle_to_swap_temp.push_back({found_blocks_no_symmetric_dict[first_hex],
                                            found_blocks_no_symmetric_dict[last_hex]});
        }
    }
    for (const auto &p : vehicle_to_swap_temp) {
        swap_hex_values_in_file_vehicle(p.first.file,
                                 p.first.first_target_position, p.first.second_target_position,
                                 p.second.file,
                                 p.second.first_target_position, p.second.second_target_position,
                                 p.first.second_target_value, p.second.second_target_value);
    }

    std::vector<std::pair<FoundBlock, FoundBlock>> weapon_to_swap_temp;
    for (const auto &pair : weapon_to_swap) {
        std::string first_hex = intToHexLittleEndian(pair.first);
        std::string last_hex = intToHexLittleEndian(pair.second);
        if (found_blocks_no_symmetric_dict.find(first_hex) != found_blocks_no_symmetric_dict.end() &&
            found_blocks_dict.find(last_hex) != found_blocks_dict.end()) {
            weapon_to_swap_temp.push_back({found_blocks_no_symmetric_dict[first_hex],
                                           found_blocks_dict[last_hex]});
        }
    }
    for (const auto &p : weapon_to_swap_temp) {
        swap_hex_values_in_file_weapon(p.first.file,
                                 p.first.first_target_position, p.first.second_target_position,
                                 p.second.file,
                                 p.second.first_target_position, p.second.second_target_position,
                                 p.first.second_target_value, p.second.first_target_value);
    }

    delete_unmodified_files("打包/uexp", modified_files);

    return 0;
}
