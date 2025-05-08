#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <filesystem>
#include <future>
#include <mutex>
#include <map>
#include <set>
#include <algorithm>

namespace fs = std::filesystem;

std::string decimal_to_little_endian_hex(int32_t decimal_number) {
    uint8_t bytes[4];
    bytes[0] = (decimal_number >> 0) & 0xFF;
    bytes[1] = (decimal_number >> 8) & 0xFF;
    bytes[2] = (decimal_number >> 16) & 0xFF;
    bytes[3] = (decimal_number >> 24) & 0xFF;
    
    std::stringstream ss;
    for (int i = 0; i < 4; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

std::vector<uint8_t> hex_string_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

bool search_bytes_in_file(const std::string& file_path, const std::vector<uint8_t>& pattern) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) return false;

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> content(size);
    if (!file.read(reinterpret_cast<char*>(content.data()), size)) return false;

    return std::search(content.begin(), content.end(), pattern.begin(), pattern.end()) != content.end();
}

std::vector<std::string> search_dat_files(const std::string& directory, const std::string& hex_str) {
    std::vector<std::string> matches;
    std::mutex mutex;
    std::vector<std::future<void>> futures;
    auto pattern = hex_string_to_bytes(hex_str);

    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            std::string path = entry.path().string();
            futures.emplace_back(std::async(std::launch::async, 
                [path, &pattern, &matches, &mutex]() {
                    if (search_bytes_in_file(path, pattern)) {
                        std::lock_guard<std::mutex> lock(mutex);
                        matches.push_back(path);
                    }
                }));
        }
    }

    for (auto& future : futures) {
        future.wait();
    }

    return matches;
}

std::map<std::string, std::string> classify_files(const std::vector<std::string>& matches, const std::string& target_hex) {
    std::map<std::string, std::string> result;
    if (matches.size() != 2) {
        for (const auto& file : matches) {
            result[file] = "未分类";
        }
        return result;
    }

    auto target_pattern = hex_string_to_bytes(target_hex);
    bool file1_has = search_bytes_in_file(matches[0], target_pattern);
    bool file2_has = search_bytes_in_file(matches[1], target_pattern);

    if (file1_has && !file2_has) {
        result[matches[0]] = "皮肤";
        result[matches[1]] = "伪实体";
    } else if (file2_has && !file1_has) {
        result[matches[1]] = "皮肤";
        result[matches[0]] = "伪实体";
    } else {
        result[matches[0]] = "未分类";
        result[matches[1]] = "未分类";
    }
    return result;
}

std::string bytes_to_hex_string(const uint8_t* data, size_t length) {
    std::stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::pair<std::string, std::string> extract_markers(const std::string& file_path, 
                                                   const std::string& hex_pattern,
                                                   const std::string& category) {
    auto pattern = hex_string_to_bytes(hex_pattern);
    std::ifstream file(file_path, std::ios::binary);
    if (!file) return {"", ""};

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> content(size);
    if (!file.read(reinterpret_cast<char*>(content.data()), size)) return {"", ""};

    auto pos = std::search(content.begin(), content.end(), pattern.begin(), pattern.end());
    if (pos == content.end()) return {"", ""};

    int position = std::distance(content.begin(), pos);
    std::string marker1, marker2;

    if (category == "皮肤") {
        int start1 = std::max(0, position - 33);
        int end1 = std::max(0, position - 31);
        int start2 = std::max(0, position - 17);
        int end2 = std::max(0, position - 15);

        marker1 = bytes_to_hex_string(content.data() + start1, end1 - start1);
        marker2 = bytes_to_hex_string(content.data() + start2, end2 - start2);
    } else if (category == "伪实体") {
        int start1 = std::max(0, position - 41);
        int end1 = std::max(0, position - 39);
        int start2 = std::max(0, position - 25);
        int end2 = std::max(0, position - 23);

        marker1 = bytes_to_hex_string(content.data() + start1, end1 - start1);
        marker2 = bytes_to_hex_string(content.data() + start2, end2 - start2);
    }

    return {marker1, marker2};
}

void update_yaml(const std::string& file_path, const std::string& marker1, const std::string& marker2) {
    std::ifstream in(file_path);
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    in.close();

    if (lines.size() >= 3) {
        lines[0] = "hex_markers:";
        lines[1] = "   start: \"" + marker1 + "\"";
        lines[2] = "   end: \"" + marker2 + "\"";
    }

    std::ofstream out(file_path);
    for (const auto& l : lines) {
        out << l << "\n";
    }
}

void update_beautification_yaml(const std::string& file_path, const std::string& skin_file) {
    std::ifstream in(file_path);
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    in.close();

    if (!lines.empty()) {
        lines[0] = "file_path: 打包/dat/" + fs::path(skin_file).filename().string();
    }

    std::ofstream out(file_path);
    for (const auto& l : lines) {
        out << l << "\n";
    }
}

int main() {
    std::vector<int32_t> decimal_numbers = {333600100};
    std::vector<std::string> hex_list;
    for (auto num : decimal_numbers) {
        hex_list.push_back(decimal_to_little_endian_hex(num));
    }

    std::string directory_to_search = "解包数据/dat";
    if (!fs::exists(directory_to_search) || !fs::is_directory(directory_to_search)) {
        return 1;
    }

    std::string skin_file, entity_file;
    std::pair<std::string, std::string> skin_markers, entity_markers;

    for (const auto& hex_str : hex_list) {
        auto matches = search_dat_files(directory_to_search, hex_str);
        auto classified = classify_files(matches, "576561706F6E5075626C6963");

        std::string target_hex = decimal_to_little_endian_hex(413753);
        for (const auto& [file, category] : classified) {
            auto [marker1, marker2] = extract_markers(file, target_hex, category);
            if (!marker1.empty() && !marker2.empty()) {
                if (category == "皮肤") {
                    skin_file = file;
                    skin_markers = {marker1, marker2};
                    update_yaml("cloth.yaml", marker1, marker2);
                } else if (category == "伪实体") {
                    entity_file = file;
                    entity_markers = {marker1, marker2};
                    update_yaml("伪实体配置.yaml", marker1, marker2);
                }
            }
        }

        if (!skin_file.empty()) {
            update_beautification_yaml("美化配置.yaml", skin_file);
        }
    }

    std::cout << "查找结果如下：\n\n";
    std::cout << "美化dat：\n";
    std::cout << "衣服美化dat小包：" 
              << (skin_file.empty() ? "未找到" : fs::path(skin_file).filename().string()) 
              << "\n";
    std::cout << "伪实体美化dat小包：" 
              << (entity_file.empty() ? "未找到" : fs::path(entity_file).filename().string()) 
              << "\n";
    std::cout << "\n特征值：\n";
    std::cout << "衣服美化：" 
              << (skin_markers.first.empty() ? "未找到" : skin_markers.first) << ", "
              << (skin_markers.second.empty() ? "未找到" : skin_markers.second) << "\n";
    std::cout << "伪实体美化：" 
              << (entity_markers.first.empty() ? "未找到" : entity_markers.first) << ", "
              << (entity_markers.second.empty() ? "未找到" : entity_markers.second) << "\n";
    std::cout << "\n所有的特征值已经在yaml中自动更新了，也就是不用手动把上面的值再写进去了\n";

    return 0;
}
