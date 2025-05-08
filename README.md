# GFP_B_SourceCode

如何在 **Termux** 环境中编译并运行 C++ 文件：

~~~markdown
# C++ 编译和运行教程（在 Termux 上）


## 文件结构

- `AutoAdd.cpp` - 处理自动添加功能
- `AutoMarker.cpp` - 处理自动标记功能
- `AutoSwitchSkin.cpp` - 处理自动切换皮肤功能
- `AutoSwitchSkinIcon.cpp` - 处理自动切换皮肤图标功能
- `Menu.cpp` - 菜单界面
- `ResetFolder.cpp` - 重置文件夹功能
- `Search.cpp` - 搜索功能
- `fast.cpp` - 快速功能
- `README.md` - 本项目的 README 文件（此文件）

## 环境准备

### 1. 安装 Termux

首先，确保你已经安装了 Termux。如果没有，可以从 F-Droid 或 Google Play 商店下载安装。

### 2. 安装编译工具

在 Termux 中，使用以下命令安装必需的编译工具（如 `g++`）：

```bash
pkg update
pkg upgrade
pkg install clang
pkg install make
~~~

### 3. 下载项目代码

在 Termux 中，你可以通过 `git` 来克隆本项目的 GitHub 仓库：

```bash
https://github.com/ELMA0158/GFP_B_SourceCode.git
cd GFP_B_SourceCode
```

### 4. 编译 C++ 文件

在项目目录下，使用 `clang++` 编译 C++ 源文件。以下是编译单个文件的命令示例：

```bash
clang++ AutoAdd.cpp -o AutoAdd
clang++ AutoMarker.cpp -o AutoMarker
clang++ AutoSwitchSkin.cpp -o AutoSwitchSkin
clang++ AutoSwitchSkinIcon.cpp -o AutoSwitchSkinIcon
clang++ Menu.cpp -o Menu
clang++ ResetFolder.cpp -o ResetFolder
clang++ Search.cpp -o Search
clang++ fast.cpp -o fast
```

这将为每个 `.cpp` 文件生成对应的可执行文件。

### 5. 运行编译后的程序

编译成功后，你可以运行生成的可执行文件。以 `AutoAdd` 为例，运行：

```bash
./AutoAdd
```

你可以使用类似的命令运行其他可执行文件：

```bash
./AutoMarker
./AutoSwitchSkin
./AutoSwitchSkinIcon
./Menu
./ResetFolder
./Search
./fast
```

### 6. 提示

- 如果你遇到权限问题，可以使用 `chmod` 来赋予执行权限：

  ```bash
  chmod +x AutoAdd
  ```

- 每次更改代码后，记得重新编译相应的 `.cpp` 文件。

## 贡献

如果你发现任何问题或有改进建议，欢迎提交 Issue 或 Pull Request。

## 许可证

本项目采用 MIT 许可证，详情请查看 LICENSE 文件。

```
### 说明：
1. **环境准备**：教程详细介绍了如何在 Termux 中安装必要的工具来编译 C++ 文件。
2. **编译和运行**：提供了如何编译 C++ 文件的命令和如何运行生成的可执行文件。
3. **贡献和许可证**：鼓励其他开发者参与到项目中来，并提到 MIT 许可证（根据实际情况可以修改）。

你可以根据自己的 GitHub 项目进一步修改文件路径或命令行示例。
```

# GFP_B_SourceCode

~~~markdown
# GFP_B_SourceCode

## How to compile and run C++ files in **Termux**:

### File Structure

- `AutoAdd.cpp` - Handles the auto-add functionality
- `AutoMarker.cpp` - Handles the auto-mark functionality
- `AutoSwitchSkin.cpp` - Handles the auto-switch skin functionality
- `AutoSwitchSkinIcon.cpp` - Handles the auto-switch skin icon functionality
- `Menu.cpp` - Menu interface
- `ResetFolder.cpp` - Resets the folder functionality
- `Search.cpp` - Search functionality
- `fast.cpp` - Fast functionality
- `README.md` - README file for this project (this file)

## Setup

### 1. Install Termux

First, ensure that you have Termux installed. If not, you can download it from F-Droid or Google Play Store.

### 2. Install the necessary compilation tools

In Termux, install the required tools such as `clang` using the following commands:

```bash
pkg update
pkg upgrade
pkg install clang
pkg install make
~~~

### 3. Download the project code

You can clone this project's GitHub repository using `git` in Termux:

```bash
git clone https://github.com/ELMA0158/GFP_B_SourceCode.git
cd GFP_B_SourceCode
```

### 4. Compile the C++ files

In the project directory, use `clang++` to compile the C++ source files. Here’s an example of how to compile each file:

```bash
clang++ AutoAdd.cpp -o AutoAdd
clang++ AutoMarker.cpp -o AutoMarker
clang++ AutoSwitchSkin.cpp -o AutoSwitchSkin
clang++ AutoSwitchSkinIcon.cpp -o AutoSwitchSkinIcon
clang++ Menu.cpp -o Menu
clang++ ResetFolder.cpp -o ResetFolder
clang++ Search.cpp -o Search
clang++ fast.cpp -o fast
```

This will generate corresponding executable files for each `.cpp` file.

### 5. Run the compiled programs

Once the compilation is successful, you can run the generated executable files. For example, to run `AutoAdd`, use:

```bash
./AutoAdd
```

You can run other executables using similar commands:

```bash
./AutoMarker
./AutoSwitchSkin
./AutoSwitchSkinIcon
./Menu
./ResetFolder
./Search
./fast
```

### 6. Tips

- If you encounter permission issues, use `chmod` to give execute permissions:

  ```bash
  chmod +x AutoAdd
  ```

- Each time you modify the code, remember to recompile the corresponding `.cpp` files.

## Contributing

If you find any issues or have suggestions for improvements, feel free to submit an Issue or a Pull Request.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

```
### Explanation:
1. **Environment Setup**: The guide provides detailed steps for setting up Termux, installing necessary tools, and preparing the environment for compiling C++ files.
2. **Compiling and Running**: It explains how to compile C++ files and run the resulting executables on Termux.
3. **Contributing and License**: Encourages contributions from other developers and mentions that the project uses the MIT License (this can be adjusted according to your actual license).

Feel free to modify the repository links or commands based on your project's specific requirements!
```
