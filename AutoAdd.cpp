#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <limits>
#include <filesystem>

namespace fs = std::filesystem;

std::pair<std::vector<int>, bool> get_swap_pair() {
    int original_code, target_code;
    bool skip = false;
    std::string input;

    std::cout << "请输入要替换的代码（跳过图标输入：s）：";
    std::cin >> input;
    
    if (input == "s") {
        skip = true;
        std::cout << "已记录，请重新输入要替换的代码：";
        std::cin >> input;
    }

    std::cout << "请输入目标代码：";
    std::string target_input;
    std::cin >> target_input;

    try {
        original_code = std::stoi(input);
        target_code = std::stoi(target_input);
        return {{original_code, target_code}, skip};
    } catch (std::exception& e) {
        std::cerr << "输入无效，请确保输入的是数字。" << std::endl;
        return {{}, false};
    }
}

void main_process() {
    std::string file_path = "cloth.yaml";
    std::string file_path2 = "伪实体配置.yaml";

    std::vector<std::string> content;
    if (fs::exists(file_path)) {
        std::ifstream file(file_path);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && line.back() == '\n') {
                    line.pop_back();
                }
                content.push_back(line);
            }
            file.close();
        }
    }

    std::vector<std::string> content2;
    if (fs::exists(file_path2)) {
        std::ifstream file(file_path2);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && line.back() == '\n') {
                    line.pop_back();
                }
                content2.push_back(line);
            }
            file.close();
        }
    } else {
        content2.push_back("search_targets:");
    }

    std::cout << "当前文件路径: " << file_path << std::endl;
    std::cout << "请输入 swap pairs，按要求格式输入：" << std::endl;

    while (true) {
        auto [swap_pair, skip] = get_swap_pair();
        if (!swap_pair.empty()) {
            std::ostringstream ss;
            ss << "   - [" << swap_pair[0] << ", " << swap_pair[1] << "]";
            content.push_back(ss.str());

            if (!skip) {
                content2.push_back(ss.str());
            }
        }

        std::cout << "是否继续输入？（1/0）：";
        std::string continue_input;
        std::cin >> continue_input;
        if (continue_input == "0") {
            break;
        }
    }

    std::ofstream file1(file_path);
    if (file1.is_open()) {
        for (size_t i = 0; i < content.size(); ++i) {
            file1 << content[i];
            if (i != content.size() - 1) file1 << '\n';
        }
        file1.close();
    }

    std::ofstream file2(file_path2);
    if (file2.is_open()) {
        for (size_t i = 0; i < content2.size(); ++i) {
            file2 << content2[i];
            if (i != content2.size() - 1) file2 << '\n';
        }
        file2.close();
    }

    std::cout << "输入完成，数对已添加到 '" << file_path << "' 和 '" << file_path2 << "'。" << std::endl;
}


int main() {
    main_process();
    return 0;
}
