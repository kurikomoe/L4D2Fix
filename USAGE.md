# 使用说明

## 前提

请确保有最新的 vcredist 运行库（x86+x64 都需要）：[传送门](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version)

![GitHub Downloads (all assets, specific tag)](https://img.shields.io/github/downloads/kurikomoe/L4D2Fix/v1.3.1/total)

## 使用方法

### 方法1：launcher

所有文件都丢到 `left4dead2.exe` 同目录下，使用 `left4dead2_fix.exe` 启动游戏（目前无法通过 steam 启动）
如果有启动项，右键 `left4dead2_fix.exe` 建立快捷方式，之后如图填写：

![image](https://github.com/user-attachments/assets/c4071491-9256-421b-911f-927a66e5aa08)

### 方法2：redirect

> [!caution]
> 由于如果因为 VAC 导致封禁，后果自负。

将所有文件放到游戏根目录（left4dead2.exe 所在目录）

将真正的游戏 exe (left4dead2.exe) 改名为 `left4dead2.orig.exe`

将 `left4dead2_fix.exe` 改名为 `left4dead2.exe`

修改 ini 文件，设置 Redirect 中的 enable = true。

在 Steam 中添加启动项 `-steam -secure`

从 Steam 启动游戏，确认弹出的警告。

该方法会允许进入 vac 服务器，但是后果自负。

## 配置文件

### kpatch.ini

kpatch.ini 默认只开启 indices buffer 修正。具体数值请自行调整。

小提示：
enable = true 或者 false

> 「首次」启动的时候会有一个弹窗，没有说明没应用上补丁。
> (不想看那个弹窗可以自行编译代码去掉，毕竟这个补丁一开始就是给那位写的。）

正常启动后会生成两个 log 文件(L4D2Fix.log, success.txt)。

## FAQ

### VAC 相关问题：

不知道，程序会改动内存，建议不要以身涉险进 VAC 服。

## 卸载

删除掉补丁引入的文件即可。
