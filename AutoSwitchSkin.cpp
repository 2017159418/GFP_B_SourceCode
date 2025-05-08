#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <cstdlib>
#include <cstdint>
#include <utility>

namespace fs = std::filesystem;

struct Config {
    std::string file_path;
    std::vector<std::pair<int, int>> swap_pairs;
};

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

Config load_config(const std::string& config_path) {
    Config config;
    std::ifstream infile(config_path);
    if (!infile) {
        std::cerr << "错误: 配置文件 '" << config_path << "' 未找到。\n";
        exit(1);
    }
    std::string line;
    bool in_swap_pairs = false;
    while (std::getline(infile, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("file_path:", 0) == 0) {
            std::string value = line.substr(std::string("file_path:").length());
            value = trim(value);
            if (!value.empty() && (value.front() == '"' || value.front() == '\'')) {
                value = value.substr(1, value.size() - 2);
            }
            config.file_path = value;
        } else if (line.rfind("swap_pairs:", 0) == 0) {
            in_swap_pairs = true;
        } else if (in_swap_pairs && line[0] == '-') {
            // 期望格式：- [123, 456]
            std::regex pair_regex("-\\s*\\[\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\]");
            std::smatch match;
            if (std::regex_search(line, match, pair_regex)) {
                int num1 = std::stoi(match[1].str());
                int num2 = std::stoi(match[2].str());
                config.swap_pairs.push_back({num1, num2});
            }
        }
    }
    return config;
}

std::vector<unsigned char> decimal_to_bytes_little_endian(int number) {
    std::vector<unsigned char> bytes(4);
    bytes[0] = static_cast<unsigned char>(number & 0xFF);
    bytes[1] = static_cast<unsigned char>((number >> 8) & 0xFF);
    bytes[2] = static_cast<unsigned char>((number >> 16) & 0xFF);
    bytes[3] = static_cast<unsigned char>((number >> 24) & 0xFF);
    return bytes;
}

std::vector<size_t> find_all_occurrences(const std::vector<unsigned char>& data, const std::vector<unsigned char>& pattern) {
    std::vector<size_t> indices;
    if (pattern.empty() || data.size() < pattern.size()) return indices;
    auto it = data.begin();
    while (it != data.end()) {
        auto pos = std::search(it, data.end(), pattern.begin(), pattern.end());
        if (pos == data.end())
            break;
        indices.push_back(std::distance(data.begin(), pos));
        it = pos + 1;
    }
    return indices;
}

std::pair<std::vector<unsigned char>, bool> swap_hex_bytes(std::vector<unsigned char> content,
                                                           const std::vector<unsigned char>& bytes1,
                                                           const std::vector<unsigned char>& bytes2) {
    if (bytes1.size() != bytes2.size()) {
        std::cerr << "错误: 十六进制值长度不同，无法互换。\n";
        return {content, false};
    }
    auto indices1 = find_all_occurrences(content, bytes1);
    auto indices2 = find_all_occurrences(content, bytes2);
    if (indices1.empty() || indices2.empty())
        return {content, false};
    size_t swap_count = std::min(indices1.size(), indices2.size());
    for (size_t i = 0; i < swap_count; i++) {
        size_t index1 = indices1[i];
        size_t index2 = indices2[i];
        for (size_t j = 0; j < bytes1.size(); j++) {
            std::swap(content[index1 + j], content[index2 + j]);
        }
    }
    return {content, true};
}

fs::path find_file_in_dir(const fs::path& start_dir, const std::string& target_file_name) {
    for (auto& p : fs::recursive_directory_iterator(start_dir)) {
        if (fs::is_regular_file(p.path()) && p.path().filename() == target_file_name) {
            return p.path();
        }
    }
    return "";
}

int main() {
    std::string config_path = "美化配置.yaml";
    Config config = load_config(config_path);
    if (config.file_path.empty()) {
        std::cerr << "错误: 配置文件中缺少 file_path 字段。\n";
        return 1;
    }
    fs::path file_path(config.file_path);
    std::string target_file_name = file_path.filename().string();

    fs::path source_dir = "解包数据/dat";
    fs::path source_path = find_file_in_dir(source_dir, target_file_name);
    if (source_path.empty()) {
        std::cerr << "错误: 未在 '" << source_dir.string() << "' 中找到文件 '" << target_file_name << "'\n";
        return 1;
    }

    fs::path destination_dir = "打包/dat";
    fs::path destination_path = destination_dir / target_file_name;
    try {
        if (!fs::exists(destination_dir))
            fs::create_directories(destination_dir);

        fs::copy_file(source_path, destination_path, fs::copy_options::overwrite_existing);
        std::cout << "文件 '" << source_path.string() << "' 已成功复制到 '" << destination_path.string() << "'\n";

        std::ifstream infile(destination_path, std::ios::binary);
        if (!infile) {
            std::cerr << "错误: 文件 '" << destination_path.string() << "' 打开失败。\n";
            return 1;
        }
        std::vector<unsigned char> content((std::istreambuf_iterator<char>(infile)),
                                             std::istreambuf_iterator<char>());
        infile.close();

        std::vector<std::pair<int, int>> failed_pairs;
        size_t total_pairs = config.swap_pairs.size();
        size_t current_pair = 0;
        for (auto& pair : config.swap_pairs) {
            current_pair++;
            std::cout << "处理第 " << current_pair << " 对，共 " << total_pairs << " 对...\n";
            std::vector<unsigned char> bytes1 = decimal_to_bytes_little_endian(pair.first);
            std::vector<unsigned char> bytes2 = decimal_to_bytes_little_endian(pair.second);
            auto result = swap_hex_bytes(content, bytes1, bytes2);
            content = result.first;
            if (!result.second) {
                failed_pairs.push_back(pair);
            }
        }

        std::ofstream outfile(destination_path, std::ios::binary);
        if (!outfile) {
            std::cerr << "错误: 无法写入文件 '" << destination_path.string() << "'\n";
            return 1;
        }
        outfile.write(reinterpret_cast<const char*>(content.data()), content.size());
        outfile.close();
        std::cout << "成功修改美化文件 '" << destination_path.string() << "'\n";

        if (!failed_pairs.empty()) {
            std::cout << "\n以下值未修改完成，请检查配置是否正确：\n";
            for (auto& pair : failed_pairs) {
                std::cout << pair.first << " ☞ " << pair.second << "\n";
            }
        } else {
            std::cout << "\n所有美化值均已成功修改。\n";
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "文件系统错误: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "发生错误: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
