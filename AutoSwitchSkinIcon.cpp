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
#include <thread>
#include <future>
#include <mutex>

namespace fs = std::filesystem;

std::set<std::string> modified_files;
std::vector<std::pair<int, int>> not_found_pairs;

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string to_hex_string(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    oss << std::hex;
    for (unsigned char c : data) {
        oss.width(2);
        oss.fill('0');
        oss << static_cast<int>(c);
    }
    return oss.str();
}

std::vector<unsigned char> hex_to_bytes(const std::string &hex) {
    std::vector<unsigned char> bytes;
    if (hex.size() % 2 != 0)
        throw std::invalid_argument("Hex string length must be even.");
    for (size_t i = 0; i < hex.size(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string decimal_to_little_endian_hex(int number, int byte_length = 4) {
    std::vector<unsigned char> bytes(byte_length);
    for (int i = 0; i < byte_length; i++) {
        bytes[i] = static_cast<unsigned char>((number >> (8 * i)) & 0xFF);
    }
    return to_hex_string(bytes);
}

std::vector<unsigned char> load_file_data(const std::string &file_path) {
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs)
        return {};
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)),
                                      std::istreambuf_iterator<char>());
}

std::vector<size_t> search_hex_positions_in_data(const std::vector<unsigned char>& data, const std::string &target_hex) {
    std::vector<size_t> positions;
    std::vector<unsigned char> pattern = hex_to_bytes(target_hex);
    if (pattern.empty() || data.size() < pattern.size())
        return positions;
    auto it = data.begin();
    while (it != data.end()) {
        auto pos_it = std::search(it, data.end(), pattern.begin(), pattern.end());
        if (pos_it == data.end())
            break;
        size_t pos = std::distance(data.begin(), pos_it);
        positions.push_back(pos);
        it = pos_it + 1;
    }
    return positions;
}

std::optional<std::string> extract_mapping_from_data(const std::vector<unsigned char>& data,
                                                     const std::string &hex_start,
                                                     const std::string &hex_end,
                                                     size_t target_position) {
    const size_t required_hex_length = 28;
    size_t required_bytes = required_hex_length / 2;
    auto positions_start = search_hex_positions_in_data(data, hex_start);
    auto positions_end = search_hex_positions_in_data(data, hex_end);
    if (positions_start.empty() || positions_end.empty())
        return std::nullopt;
    auto closest_start = *std::min_element(positions_start.begin(), positions_start.end(),
        [target_position](size_t a, size_t b) {
            return std::abs((int)a - (int)target_position) < std::abs((int)b - (int)target_position);
        });
    auto closest_end = *std::min_element(positions_end.begin(), positions_end.end(),
        [target_position](size_t a, size_t b) {
            return std::abs((int)a - (int)target_position) < std::abs((int)b - (int)target_position);
        });
    if (closest_start > closest_end)
        std::swap(closest_start, closest_end);
    size_t marker_length = hex_start.size() / 2;
    if (closest_start + marker_length > data.size())
        return std::nullopt;
    size_t middle_length = closest_end - (closest_start + marker_length);
    if (middle_length != required_bytes)
        return std::nullopt;
    std::vector<unsigned char> middle_data(data.begin() + closest_start + marker_length,
                                             data.begin() + closest_end);
    std::string mapping = to_hex_string(middle_data);
    if (mapping.size() != required_hex_length)
        return std::nullopt;
    return mapping;
}

bool write_mapping(const std::string &file_path,
                   const std::string &start_marker,
                   const std::string &end_marker,
                   size_t target_position,
                   const std::string &new_mapping) {
    std::vector<unsigned char> data = load_file_data(file_path);
    if (data.empty())
        return false;
    auto positions_start = search_hex_positions_in_data(data, start_marker);
    auto positions_end = search_hex_positions_in_data(data, end_marker);
    if (positions_start.empty() || positions_end.empty())
        return false;
    auto closest_start = *std::min_element(positions_start.begin(), positions_start.end(),
        [target_position](size_t a, size_t b) {
            return std::abs((int)a - (int)target_position) < std::abs((int)b - (int)target_position);
        });
    auto closest_end = *std::min_element(positions_end.begin(), positions_end.end(),
        [target_position](size_t a, size_t b) {
            return std::abs((int)a - (int)target_position) < std::abs((int)b - (int)target_position);
        });
    if (closest_start > closest_end)
        std::swap(closest_start, closest_end);
    size_t marker_length = start_marker.size() / 2;
    size_t original_length = closest_end - (closest_start + marker_length);
    auto new_bytes = hex_to_bytes(new_mapping);
    if (new_bytes.size() != original_length)
        throw std::runtime_error("新的映射长度与原映射长度不匹配。");
    std::copy(new_bytes.begin(), new_bytes.end(), data.begin() + closest_start + marker_length);
    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs)
        return false;
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

struct Config {
    std::string folder_path;
    std::vector<std::pair<int, int>> search_targets;
    std::string hex_marker_start = "aa78";
    std::string hex_marker_end = "9e78";
};

Config load_config(const std::string &config_file) {
    Config cfg;
    std::ifstream ifs(config_file);
    if (!ifs) {
        std::exit(1);
    }
    std::string line;
    bool in_targets = false;
    bool in_hex_markers = false;
    std::regex target_regex("-\\s*\\[\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\]");
    std::regex key_value_regex("^\\s*(\\w+)\\s*:\\s*\"(.*?)\"");
    std::regex key_value_noquote("^\\s*(\\w+)\\s*:\\s*(\\S+)");
    while (std::getline(ifs, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line.rfind("folder_path:", 0) == 0) {
            std::smatch m;
            if (std::regex_search(line, m, key_value_regex))
                cfg.folder_path = m[2];
            else if (std::regex_search(line, m, key_value_noquote))
                cfg.folder_path = m[2];
            in_targets = false;
            in_hex_markers = false;
        } else if (line.rfind("search_targets:", 0) == 0) {
            in_targets = true;
            in_hex_markers = false;
        } else if (line.rfind("hex_markers:", 0) == 0) {
            in_hex_markers = true;
            in_targets = false;
        } else if (in_targets && line[0] == '-') {
            std::smatch m;
            if (std::regex_search(line, m, target_regex)) {
                int a = std::stoi(m[1].str());
                int b = std::stoi(m[2].str());
                cfg.search_targets.push_back({a, b});
            }
        } else if (in_hex_markers) {
            std::smatch m;
            if (std::regex_search(line, m, key_value_regex)) {
                std::string key = m[1];
                std::string value = m[2];
                if (key == "start")
                    cfg.hex_marker_start = value;
                else if (key == "end")
                    cfg.hex_marker_end = value;
            } else if (std::regex_search(line, m, key_value_noquote)) {
                std::string key = m[1];
                std::string value = m[2];
                if (key == "start")
                    cfg.hex_marker_start = value;
                else if (key == "end")
                    cfg.hex_marker_end = value;
            }
        }
    }
    return cfg;
}

void prepare_destination() {
    fs::path cwd = fs::current_path();
    fs::path dest_dir = cwd / "打包" / "uexp";
    fs::path source_dir = cwd / "解包数据" / "uexp";
    if (fs::exists(dest_dir))
        fs::remove_all(dest_dir);
    fs::create_directories(dest_dir);
    if (fs::exists(source_dir)) {
        for (auto &entry : fs::recursive_directory_iterator(source_dir)) {
            if (fs::is_regular_file(entry.path())) {
                fs::path dest_file = dest_dir / entry.path().filename();
                fs::copy_file(entry.path(), dest_file, fs::copy_options::overwrite_existing);
            }
        }
    }
}

struct MappingInfo {
    std::string file;
    size_t target_position;
    std::string mapping;
};

//
// 多线程版本：扫描指定目录及其子目录中所有文件，对每个文件尝试提取目标代码的映射信息。
// 返回一个映射：code -> MappingInfo
//
std::unordered_map<int, MappingInfo> scan_all_files_mt(const std::string &root_path,
                                                       const std::vector<std::pair<int, int>> &targets,
                                                       const std::string &hex_start,
                                                       const std::string &hex_end) {
    std::set<int> codes_set;
    for (const auto &group : targets) {
        codes_set.insert(group.first);
        codes_set.insert(group.second);
    }
    std::vector<std::string> all_files;
    for (auto &entry : fs::recursive_directory_iterator(root_path)) {
        if (fs::is_regular_file(entry.path()))
            all_files.push_back(entry.path().string());
    }
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
        num_threads = 4;
    size_t total_files = all_files.size();
    size_t chunk_size = (total_files + num_threads - 1) / num_threads;
    std::vector<std::future<std::unordered_map<int, MappingInfo>>> futures;
    for (size_t i = 0; i < num_threads; i++) {
        auto start_it = all_files.begin() + i * chunk_size;
        auto end_it = (i == num_threads - 1) ? all_files.end() : start_it + chunk_size;
        std::vector<std::string> chunk(start_it, end_it);
        futures.push_back(std::async(std::launch::async, [chunk, &codes_set, &hex_start, &hex_end]() -> std::unordered_map<int, MappingInfo> {
            std::unordered_map<int, MappingInfo> local_mapping;
            for (const auto &file_path : chunk) {
                auto data = load_file_data(file_path);
                if (data.empty())
                    continue;
                for (int code : codes_set) {
                    if (local_mapping.find(code) != local_mapping.end())
                        continue;
                    std::string code_hex = decimal_to_little_endian_hex(code, 4);
                    auto positions = search_hex_positions_in_data(data, code_hex);
                    if (positions.empty())
                        continue;
                    size_t target_pos = positions[0];
                    auto mappingOpt = extract_mapping_from_data(data, hex_start, hex_end, target_pos);
                    if (mappingOpt.has_value()) {
                        local_mapping[code] = MappingInfo{ fs::absolute(file_path).string(), target_pos, mappingOpt.value() };
                    }
                }
            }
            return local_mapping;
        }));
    }
    std::unordered_map<int, MappingInfo> mapping_info;
    for (auto &fut : futures) {
        auto local_map = fut.get();
        for (const auto &p : local_map) {
            if (mapping_info.find(p.first) == mapping_info.end())
                mapping_info[p.first] = p.second;
        }
    }
    return mapping_info;
}

//
// 多线程版本：扫描所有文件，对于配置中每组目标代码（2个数字），即使映射分布在不同文件中，也交换它们的映射数据，并写回原文件。
// 为防止多个线程同时写入同一文件，使用了每个文件对应的互斥量（按文件名排序锁定，避免死锁）。
//
void process_cross_file_swap_mt(const std::string &root_path,
                                const std::vector<std::pair<int, int>> &targets,
                                const std::string &hex_start,
                                const std::string &hex_end) {
    auto mapping_info = scan_all_files_mt(root_path, targets, hex_start, hex_end);
    // 为所有参与交换的文件建立互斥量
    std::unordered_map<std::string, std::mutex> file_mutexes;
    for (const auto &group : targets) {
        if (mapping_info.find(group.first) != mapping_info.end())
            file_mutexes[mapping_info[group.first].file];
        if (mapping_info.find(group.second) != mapping_info.end())
            file_mutexes[mapping_info[group.second].file];
    }
    std::mutex global_mutex; // 用于保护 not_found_pairs 和 modified_files 的更新
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
        num_threads = 4;
    size_t total_targets = targets.size();
    size_t chunk_size = (total_targets + num_threads - 1) / num_threads;
    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < num_threads; i++) {
        size_t start_index = i * chunk_size;
        size_t end_index = std::min(start_index + chunk_size, total_targets);
        futures.push_back(std::async(std::launch::async, [start_index, end_index, &targets, &mapping_info, &hex_start, &hex_end, &file_mutexes, &global_mutex]() {
            for (size_t j = start_index; j < end_index; j++) {
                int code1 = targets[j].first, code2 = targets[j].second;
                if (mapping_info.find(code1) == mapping_info.end() || mapping_info.find(code2) == mapping_info.end()) {
                    std::lock_guard<std::mutex> lock(global_mutex);
                    not_found_pairs.push_back({code1, code2});
                    continue;
                }
                const MappingInfo &mi1 = mapping_info.at(code1);
                const MappingInfo &mi2 = mapping_info.at(code2);
                // 如果两个映射在同一文件中，仅锁定该文件的互斥量
                if (mi1.file == mi2.file) {
                    std::lock_guard<std::mutex> lock(file_mutexes[mi1.file]);
                    try {
                        if (write_mapping(mi1.file, hex_start, hex_end, mi1.target_position, mi2.mapping))
                            modified_files.insert(fs::absolute(mi1.file).string());
                        if (write_mapping(mi2.file, hex_start, hex_end, mi2.target_position, mi1.mapping))
                            modified_files.insert(fs::absolute(mi2.file).string());
                    } catch (const std::exception &) {
                        continue;
                    }
                } else {
                    // 为避免死锁，按文件名顺序锁定两个文件的互斥量
                    std::string fileA = mi1.file, fileB = mi2.file;
                    if (fileA > fileB)
                        std::swap(fileA, fileB);
                    std::unique_lock<std::mutex> lock1(file_mutexes[fileA], std::defer_lock);
                    std::unique_lock<std::mutex> lock2(file_mutexes[fileB], std::defer_lock);
                    std::lock(lock1, lock2);
                    try {
                        if (write_mapping(mi1.file, hex_start, hex_end, mi1.target_position, mi2.mapping))
                            modified_files.insert(fs::absolute(mi1.file).string());
                        if (write_mapping(mi2.file, hex_start, hex_end, mi2.target_position, mi1.mapping))
                            modified_files.insert(fs::absolute(mi2.file).string());
                    } catch (const std::exception &) {
                        continue;
                    }
                }
            }
        }));
    }
    for (auto &f : futures) {
        f.get();
    }
}

void move_and_cleanup(const std::string &source_dir) {
    for (auto &entry : fs::recursive_directory_iterator(source_dir)) {
        if (fs::is_regular_file(entry.path())) {
            std::string abs_path = fs::absolute(entry.path()).string();
            if (modified_files.find(abs_path) == modified_files.end())
                fs::remove(entry.path());
        }
    }
    for (auto it = fs::recursive_directory_iterator(source_dir);
         it != fs::recursive_directory_iterator(); ) {
        if (fs::is_directory(it->path()) && fs::is_empty(it->path())) {
            fs::remove(it->path());
            it = fs::recursive_directory_iterator(source_dir);
        } else {
            ++it;
        }
    }
}


int main() {
    std::string config_file = "伪实体配置.yaml";
    Config config = load_config(config_file);
    if (config.folder_path.empty())
        return 1;
    prepare_destination();
    process_cross_file_swap_mt(config.folder_path, config.search_targets,
                               config.hex_marker_start, config.hex_marker_end);
    move_and_cleanup(config.folder_path);
    return 0;
}
