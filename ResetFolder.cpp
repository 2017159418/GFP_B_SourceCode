#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main() {

    fs::path parent_dir = fs::current_path();

    fs::path pack_dir = parent_dir / "打包";
    fs::path unpack_dir = parent_dir / "解包数据";

    if (fs::exists(pack_dir)) {
        fs::remove_all(pack_dir);
    }
    if (fs::exists(unpack_dir)) {
        fs::remove_all(unpack_dir);
    }

    fs::create_directories(pack_dir);
    fs::create_directories(unpack_dir);

    fs::create_directories(pack_dir / "dat");
    fs::create_directories(pack_dir / "uexp");
    fs::create_directories(unpack_dir / "dat");
    fs::create_directories(unpack_dir / "uexp");

    std::cout << "文件夹已成功创建！" << std::endl;
    return 0;
}
