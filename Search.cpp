#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cctype>

namespace fs = std::filesystem;

std::vector<unsigned char> decimalToLittleEndianBytes(int32_t num, size_t byte_length = 4) {
    std::vector<unsigned char> bytes(byte_length);
    for (size_t i = 0; i < byte_length; i++) {
        bytes[i] = static_cast<unsigned char>((num >> (8 * i)) & 0xFF);
    }
    return bytes;
}


bool searchHexInFile(const std::string& filePath, const std::vector<unsigned char>& bytePattern) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "无法读取文件 " << filePath << std::endl;
        return false;
    }
    std::vector<unsigned char> content((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
    if (content.size() < bytePattern.size()) return false;
    auto it = std::search(content.begin(), content.end(),
                          bytePattern.begin(), bytePattern.end());
    return it != content.end();
}

void workerFunction(const std::vector<std::string>& files,
                    const std::vector<unsigned char>& bytePattern,
                    std::atomic<size_t>& progress,
                    std::vector<std::string>& matches,
                    std::mutex& matchesMutex,
                    size_t start, size_t end)
{
    for (size_t i = start; i < end; i++) {
        if (searchHexInFile(files[i], bytePattern)) {
            std::lock_guard<std::mutex> lock(matchesMutex);
            matches.push_back(files[i]);
        }
        progress++;
    }
}

int main() {

    std::cout << "请输入要搜索的 .dat 文件所在的目录路径（留空使用默认路径 '解包数据/dat'）: ";
    std::string directoryToSearch;
    std::getline(std::cin, directoryToSearch);
    if (directoryToSearch.empty()) {
        directoryToSearch = "解包数据/dat";
    }
    if (!fs::exists(directoryToSearch) || !fs::is_directory(directoryToSearch)) {
        std::cerr << "错误: 目录 '" << directoryToSearch << "' 不存在。" << std::endl;
        return 1;
    }

    std::vector<int32_t> decimalNumbers = {333600100};
    std::vector<unsigned char> bytePattern = decimalToLittleEndianBytes(decimalNumbers[0]);

    std::vector<std::string> datFiles;
    for (const auto& entry : fs::recursive_directory_iterator(directoryToSearch)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".dat") {
                datFiles.push_back(entry.path().string());
            }
        }
    }
    if (datFiles.empty()) {
        std::cout << "未找到任何 .dat 文件。" << std::endl;
        return 0;
    }

    const size_t numThreads = 8;
    std::atomic<size_t> progress(0);
    std::vector<std::string> matches;
    std::mutex matchesMutex;
    size_t totalFiles = datFiles.size();
    std::vector<std::thread> workers;
    size_t filesPerThread = (totalFiles + numThreads - 1) / numThreads;

    for (size_t t = 0; t < numThreads; t++) {
        size_t start = t * filesPerThread;
        size_t end = std::min(start + filesPerThread, totalFiles);
        if (start < end) {
            workers.emplace_back(workerFunction,
                                 std::cref(datFiles),
                                 std::cref(bytePattern),
                                 std::ref(progress),
                                 std::ref(matches),
                                 std::ref(matchesMutex),
                                 start, end);
        }
    }

    while (progress < totalFiles) {
        std::cout << "\r搜索进度: " << progress.load() << "/" << totalFiles << " 个文件已处理" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "\r搜索进度: " << totalFiles << "/" << totalFiles << " 个文件已处理" << std::endl;

    for (auto& th : workers) {
        if (th.joinable()) {
            th.join();
        }
    }

    std::cout << "找到包含美化的文件:" << std::endl;
    for (const auto& match : matches) {
        std::cout << " - " << match << std::endl;
    }
    std::cout << "搜索完毕。" << std::endl;
    return 0;
}
