#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <thread>
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;


const std::string BLUE    = "\033[0;34m";
const std::string GREEN   = "\033[0;32m";
const std::string CYAN    = "\033[0;36m";
const std::string YELLOW  = "\033[1;33m";
const std::string MAGENTA = "\033[0;35m";
const std::string RED     = "\033[0;31m";
const std::string WHITE   = "\033[0;37m";
const std::string RESET   = "\033[0m";
const std::string BOLD    = "\033[1m";
const std::string SAKURA_PINK = "\033[38;5;218m"; // 亮粉色
const std::string LIGHT_MAGENTA = "\033[1;35m"; // 亮洋红


const std::string DEFAULT_PAK_DIR = "pak";


void clearScreen() {
    std::system("clear");
}


void show_loading() {
    const std::string spinner = "|/-\\";
    for (int i = 0; i < 30; ++i) {
        std::cout << " [" << spinner[i % spinner.size()] << "]  " << "\r" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "    完成!" << std::endl;
}

void show_main_menu() {
    if (fs::exists("菜单信息")) {
        std::ifstream infile("菜单信息");
        std::string line;
        while (std::getline(infile, line)) {
            std::cout << line << std::endl;
        }
    } else {
        std::cout << RED << "未找到菜单信息文件！" << RESET << std::endl;
    }
    std::cout << GREEN   << "1) 自动美化工具"      << RESET << std::endl;
    std::cout << BLUE    << "2) 解包工具"         << RESET << std::endl;
    std::cout << MAGENTA << "3) 打包工具"         << RESET << std::endl;
    std::cout << WHITE   << "4) 自动搜索特征值"     << RESET << std::endl;
    std::cout << YELLOW  << "5) 搜索工具"         << RESET << std::endl;
    std::cout << RED     << "6) 重置文件夹"       << RESET << std::endl;
    std::cout << SAKURA_PINK     << "7) 自动添加"       << RESET << std::endl;
    std::cout << CYAN    << "8) 退出工具"         << RESET << std::endl;
    std::cout << BOLD    << "请选择操作的序号："  << RESET;
}


std::vector<std::string> list_pak_files(const std::string &dir) {
    std::vector<std::string> files;
    if (!fs::exists(dir)) return files;
    for (const auto &entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".pak") {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

bool select_pak_file(const std::string &dir, std::string &selected_file) {
    auto files = list_pak_files(dir);
    if (files.empty()) {
        std::cout << RED << "目录 \"" << dir << "\" 中没有找到 .pak 文件" << RESET << std::endl;
        return false;
    }
    std::cout << YELLOW << "请选择要操作的 .pak 文件：" << RESET << std::endl;
    for (size_t i = 0; i < files.size(); ++i) {
        std::cout << YELLOW << (i + 1) << ") " << files[i] << RESET << std::endl;
    }
    std::cout << BOLD << "请输入选项：" << RESET;
    int choice = 0;
    std::cin >> choice;
    if (choice >= 1 && choice <= static_cast<int>(files.size())) {
        selected_file = files[choice - 1];
        std::cout << GREEN << "你选择了：" << selected_file << RESET << std::endl;
        return true;
    } else {
        std::cout << RED << "无效选择！" << RESET << std::endl;
        return false;
    }
}

int main() {

    std::string PAK_DIR = DEFAULT_PAK_DIR;

    if (!fs::exists(PAK_DIR)) {
        std::cout << RED << "目录 \"" << PAK_DIR << "\" 不存在！" << RESET << std::endl;
        return 1;
    }

    while (true) {
        clearScreen();
        show_main_menu();
        int choice = 0;
        std::cin >> choice;
        std::cout << std::endl;
        switch (choice) {
            case 1: {
                clearScreen();
                std::cout << GREEN << "自动美化工具" << RESET << std::endl;
                while (true) {
                    std::cout << YELLOW << "1) 自动美化(衣服)" << RESET << std::endl;
                    std::cout << YELLOW << "2) 快速美化(衣服)" << RESET << std::endl;
                    std::cout << YELLOW << "3) 自动美化(衣服图标)" << RESET << std::endl;
                    std::cout << BLUE << "4) 返回主菜单" << RESET << std::endl;
                    std::cout << BOLD << "请选择对应序号：" << RESET;
                    int sub_choice = 0;
                    std::cin >> sub_choice;
                    if (sub_choice == 1) {
                        std::cout << YELLOW << "开始自动美化衣服..." << RESET << std::endl;
                        std::system("./tools/AutoSwitchSkin");
                        show_loading();
                    } else if (sub_choice == 2) {
                        std::cout << YELLOW << "开始自动美化衣服..." << RESET << std::endl;
                        std::system("./tools/fast");
                        show_loading();
                    } else if (sub_choice == 3) {
                        std::cout << YELLOW << "开始自动美化衣服图标..." << RESET << std::endl;
                        std::system("./tools/AutoSwitchSkinIcon");
                        show_loading();
                    } else if (sub_choice == 4) {
                        clearScreen();
                        break;
                    } else {
                        std::cout << RED << "请输入 1 到 4 之间的数字！" << RESET << std::endl;
                    }
                    std::cout << "按任意键继续..." << std::endl;
                    std::cin.ignore();
                    std::cin.get();
                }
                break;
            }
            case 2: {
                clearScreen();
                std::cout << BLUE << "解包工具" << RESET << std::endl;
                while (true) {
                    std::cout << CYAN << "1) dat 解包" << RESET << std::endl;
                    std::cout << CYAN << "2) uexp 解包" << RESET << std::endl;
                    std::cout << YELLOW << "3) 返回主菜单" << RESET << std::endl;
                    std::cout << BOLD << "请选择：" << RESET;
                    int unpack_choice = 0;
                    std::cin >> unpack_choice;
                    if (unpack_choice == 1) {
                        std::string selected_file;
                        if (select_pak_file(PAK_DIR, selected_file)) {
                            std::cout << YELLOW << "正在解包 .dat 文件..." << RESET << std::endl;
                            // 调用外部命令：注意引号用法
                            std::string cmd = "qemu-i386 tools/quickbms tools/Extract.bms \"" + selected_file + "\" \"解包数据/dat\"";
                            std::system(cmd.c_str());
                            show_loading();
                        }
                    } else if (unpack_choice == 2) {
                        std::string selected_file;
                        if (select_pak_file(PAK_DIR, selected_file)) {
                            std::cout << YELLOW << "正在解包 .uexp 文件..." << RESET << std::endl;
                            std::string cmd = "./tools/unpack -a \"" + selected_file + "\" \"解包数据/uexp\"";
                            std::system(cmd.c_str());
                            show_loading();
                        }
                    } else if (unpack_choice == 3) {
                        clearScreen();
                        break;
                    } else {
                        std::cout << RED << "请输入 1 到 3 之间的数字！" << RESET << std::endl;
                    }
                    std::cout << "按任意键返回..." << std::endl;
                    std::cin.ignore();
                    std::cin.get();
                }
                break;
            }
            case 3: {
                clearScreen();
                std::cout << MAGENTA << "打包工具" << RESET << std::endl;
                while (true) {
                    std::cout << CYAN << "1) dat 打包" << RESET << std::endl;
                    std::cout << CYAN << "2) uexp 打包" << RESET << std::endl;
                    std::cout << YELLOW << "3) 返回主菜单" << RESET << std::endl;
                    std::cout << BOLD << "请选择：" << RESET;
                    int pack_choice = 0;
                    std::cin >> pack_choice;
                    if (pack_choice == 1) {
                        std::string selected_file;
                        if (select_pak_file(PAK_DIR, selected_file)) {
                            std::cout << YELLOW << "正在打包 .dat 文件..." << RESET << std::endl;
                            std::string cmd = "qemu-i386 tools/quickbms -w -r -r tools/Pack.bms \"" + selected_file + "\" \"打包/dat\"";
                            std::system(cmd.c_str());
                            show_loading();
                        }
                    } else if (pack_choice == 2) {
                        std::string selected_file;
                        if (select_pak_file(PAK_DIR, selected_file)) {
                            std::cout << YELLOW << "正在打包 .uexp 文件..." << RESET << std::endl;
                            std::string cmd = "./tools/unpack -a -r \"" + selected_file + "\" \"打包/uexp\"";
                            std::system(cmd.c_str());
                            show_loading();
                        }
                    } else if (pack_choice == 3) {
                        clearScreen();
                        break;
                    } else {
                        std::cout << RED << "请输入 1 到 3 之间的数字！" << RESET << std::endl;
                    }
                    std::cout << "按任意键返回..." << std::endl;
                    std::cin.ignore();
                    std::cin.get();
                }
                break;
            }
            case 4: {
                clearScreen();
                std::cout << WHITE << "自动搜索特征值..." << RESET << std::endl;
                std::system("./tools/AutoMarker");
                break;
            }
            case 5: {
                clearScreen();
                std::cout << YELLOW << "搜索工具" << RESET << std::endl;
                std::cout << CYAN << "开始搜索..." << RESET << std::endl;
                std::system("./tools/Search");
                show_loading();
                break;
            }
            case 6: {
                clearScreen();
                std::cout << RED << "重置文件夹..." << RESET << std::endl;
                std::system("./tools/ResetFolder");
                break;
            }
            case 7: {
                clearScreen();
                std::cout << SAKURA_PINK << "重自动添加..." << RESET << std::endl;
                std::system("./tools/AutoAdd");
                break;
            }
            case 8: {
                std::cout << RED << "退出工具..." << RESET << std::endl;
                return 0;
            }
            default:
                std::cout << RED << "请输入 1 到 8 之间的数字！" << RESET << std::endl;
                break;
        }
        std::cout << "按任意键返回主菜单..." << std::endl;
        std::cin.ignore();
        std::cin.get();
    }
    return 0;
}
