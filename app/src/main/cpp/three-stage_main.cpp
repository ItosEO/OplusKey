#include "three-stage_main.h"
#include "logger.h"
#include <string>
#include <unistd.h>
#include "utils.h"
#include <poll.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <cstring>

// 优化版本：使用 pread() 避免 lseek，减少系统调用
static std::string readTriStateFromFd(int fd, bool& ok) {
    if (fd < 0) {
        ok = false;
        return {};
    }

    char buf[8];
    // 使用 pread() 从文件开头读取，无需 lseek
    ssize_t n = pread(fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        ok = false;
        return {};
    }
    buf[n] = '\0';

    // 内联去除尾部空白字符，避免 string 操作
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r' || buf[n-1] == ' ' || buf[n-1] == '\t')) {
        buf[--n] = '\0';
    }
    
    ok = true;
    return std::string(buf, n);
}

// 检查设备是否支持指定 KEY（这里保留 KEY_F3）
static bool deviceSupportsKeyCode(int fd, int keyCode) {
    // KEY_MAX 可能较大，按位存储
    constexpr int kKeyBitsSize = (KEY_MAX + 8) / 8;
    unsigned char keyBits[kKeyBitsSize];
    std::memset(keyBits, 0, sizeof(keyBits));

    // 读取 EV_KEY 能力位
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0) {
        return false;
    }
    int byteIndex = keyCode / 8;
    int bitIndex = keyCode % 8;
    if (byteIndex >= (int)sizeof(keyBits)) {
        return false;
    }
    return (keyBits[byteIndex] & (1u << bitIndex)) != 0;
}

// 参考 service_decrypted.sh 的方案：找到支持 KEY_F3 的第一个设备
// 类似 getevent -lp | awk '/^add device/ {dev=$NF} /KEY_F3/ {print dev; exit}'
static std::string findKeyF3Device() {
    DIR* dir = opendir("/dev/input");
    if (!dir) return "";

    struct dirent* ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        // 仅匹配以 "event" 开头的节点
        if (std::strncmp(ent->d_name, "event", 5) != 0) continue;

        std::string path = std::string("/dev/input/") + ent->d_name;
        int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) continue;

        bool supportsF3 = deviceSupportsKeyCode(fd, KEY_F3);
        close(fd);
        
        if (supportsF3) {
            closedir(dir);
            return path;
        }
    }
    closedir(dir);
    return "";
}

void threeStageMain(const std::string& mod_dir) {
    // 参考 service_decrypted.sh: 找到支持 KEY_F3 的设备
    std::string devicePath;
    int inputFd = -1;
    
    // 循环尝试找到设备并打开
    while (true) {
        devicePath = findKeyF3Device();
        if (devicePath.empty()) {
            LOG_WARN("未发现支持 KEY_F3 的输入设备，1秒后重试");
            sleep(1);
            continue;
        }
        
        LOG_INFO("找到 KEY_F3 设备: " + devicePath);
        
        inputFd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (inputFd < 0) {
            LOG_WARN("无法打开设备 " + devicePath + "，1秒后重试");
            sleep(1);
            continue;
        }
        break;
    }

    struct pollfd pfd{};
    pfd.fd = inputFd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    // tri_state fd 在事件循环生命周期内持久化
    int triFd = open("/proc/tristatekey/tri_state", O_RDONLY | O_CLOEXEC);
    if (triFd < 0) {
        LOG_WARN("无法打开 /proc/tristatekey/tri_state，启动后将于首次 F3 尝试重开");
    }

    // 初始化 tri_state：启动阶段尽量读取基线，多次重试；若仍失败，等待事件后再初始化
    std::string lastValue;
    bool hasInitial = false;
    {
        const int kInitAttempts = 10;
        for (int attempt = 0; attempt < kInitAttempts && !hasInitial; ++attempt) {
            bool ok = false;
            if (triFd < 0) {
                triFd = open("/proc/tristatekey/tri_state", O_RDONLY | O_CLOEXEC);
            }
            std::string initVal;
            if (triFd >= 0) {
                initVal = readTriStateFromFd(triFd, ok);
            }
            if (ok) {
                lastValue = initVal;
                hasInitial = true;
                LOG_INFO(std::string("三段键初始值: ") + initVal);
                break;
            }
            if (triFd >= 0) { close(triFd); triFd = -1; }
            usleep(100 * 1000); // 100ms 后重试
        }
        if (!hasInitial) {
            LOG_WARN("启动阶段未能读取 tri_state，等待事件后再初始化");
        }
    }

    // 事件循环：参考 shell 版本的单设备监听
    while (true) {
        int ret = poll(&pfd, 1, -1); // 无限等待
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("poll 失败，退出事件循环");
            break;
        }
        if (ret == 0) continue; // 不应该发生（无限等待）

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            LOG_WARN("输入设备出现错误，fd=" + std::to_string(pfd.fd) + "，尝试重新打开");
            close(inputFd);
            
            // 重新查找并打开设备
            while (true) {
                devicePath = findKeyF3Device();
                if (devicePath.empty()) {
                    LOG_WARN("重扫时未发现支持 KEY_F3 的设备，1秒后重试");
                    sleep(1);
                    continue;
                }
                inputFd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
                if (inputFd < 0) {
                    LOG_WARN("重新打开设备失败，1秒后重试");
                    sleep(1);
                    continue;
                }
                pfd.fd = inputFd;
                break;
            }
            continue;
        }
        if (!(pfd.revents & POLLIN)) {
            pfd.revents = 0;
            continue;
        }

        // 读取并处理所有可用事件
        struct input_event ev{};
        ssize_t n = 0;
        while ((n = read(pfd.fd, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
            // 只处理 KEY_F3 按下事件（value=1），忽略释放事件（value=0）减少无效读取
            if (ev.type == EV_KEY && ev.code == KEY_F3 && ev.value == 1) {
                // 捕获 KEY_F3：优化的 tri_state 读取
                bool ok = false;
                std::string curr;
                
                // 尝试从现有 fd 读取
                if (triFd >= 0) {
                    curr = readTriStateFromFd(triFd, ok);
                }
                
                // 仅在读取失败时重新打开一次，避免频繁 close/open
                if (!ok) {
                    if (triFd >= 0) { 
                        close(triFd); 
                        triFd = -1; 
                    }
                    triFd = open("/proc/tristatekey/tri_state", O_RDONLY | O_CLOEXEC);
                    if (triFd >= 0) {
                        curr = readTriStateFromFd(triFd, ok);
                    }
                    
                    if (!ok) {
                        LOG_WARN("读取 /proc/tristatekey/tri_state 失败，跳过此次事件");
                        continue;
                    }
                }

                if (!hasInitial) {
                    // 首次初始化，不触发脚本
                    lastValue = curr;
                    hasInitial = true;
                    LOG_INFO(std::string("初始化三段键状态: ") + curr);
                    continue;
                }

                if (curr != lastValue) {
                    std::string arg;
                    if (curr == "1") arg = "up";
                    else if (curr == "2") arg = "center";
                    else if (curr == "3") arg = "down";

                    if (!arg.empty()) {
                        LOG_INFO(std::string("三段键状态变化: [") + lastValue + "] -> [" + curr + "]，执行 t-stage.sh 参数: " + arg);
                        execBackground(DEFAULT_SHELL, {mod_dir + "/t-stage.sh", arg});
                    } else {
                        LOG_WARN(std::string("未知的三段键值，跳过执行: ") + curr);
                    }
                    lastValue = curr;
                }
            }
        }

        if (n < 0 && errno != EAGAIN) {
            LOG_WARN("读取输入设备失败，fd=" + std::to_string(pfd.fd));
        }
        pfd.revents = 0;
    }

    // 清理：关闭 fd
    if (inputFd >= 0) close(inputFd);
    if (triFd >= 0) close(triFd);
    LOG_INFO("三段键事件监听已退出");
}

void threeStageExit() {
    // no-op：退出时机由线程/进程销毁或致命 I/O 错误决定
}