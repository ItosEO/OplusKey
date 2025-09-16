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
#include <vector>
#include <cstring>

static std::string readTriStateFromFd(int fd, bool& ok) {
    if (fd < 0) {
        ok = false;
        return {};
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        ok = false;
        return {};
    }

    char buf[8];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        ok = false;
        return {};
    }
    buf[n] = '\0';

    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
        line.pop_back();
    }
    ok = true;
    return line;
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

// 扫描 /dev/input/event* 并打开支持 KEY_F3 的设备
static std::vector<int> scanAndOpenKeyDevices() {
    std::vector<int> fds;
    DIR* dir = opendir("/dev/input");
    if (!dir) return fds;

    struct dirent* ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        // 仅匹配以 "event" 开头的节点
        if (std::strncmp(ent->d_name, "event", 5) != 0) continue;

        std::string path = std::string("/dev/input/") + ent->d_name;
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;

        if (deviceSupportsKeyCode(fd, KEY_F3)) {
            fds.push_back(fd);
        } else {
            close(fd);
        }
    }
    closedir(dir);
    return fds;
}

static void closeInputFds(std::vector<int>& fds) {
    for (int fd : fds) {
        if (fd >= 0) close(fd);
    }
    fds.clear();
}

void threeStageMain(const std::string& mod_dir) {
    // 动态扫描并打开所有支持 KEY_F3 的输入设备
    std::vector<int> inputFds = scanAndOpenKeyDevices();
    if (inputFds.empty()) {
        LOG_WARN("未发现支持 KEY_F3 的输入设备，稍后将重试");
    }

    // 构建 poll 列表
    auto buildPollFds = [](const std::vector<int>& fds) {
        std::vector<struct pollfd> pfds;
        pfds.reserve(fds.size());
        for (int fd : fds) {
            struct pollfd p{};
            p.fd = fd;
            p.events = POLLIN;
            p.revents = 0;
            pfds.push_back(p);
        }
        return pfds;
    };
    std::vector<struct pollfd> pfds = buildPollFds(inputFds);

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

    // 事件循环：多路监听。仅在 KEY_F3 时读取 tri_state 并按变化触发脚本
    bool shouldExit = false;
    const int kPollTimeoutMs = 3000; // 周期性允许我们执行重扫
    while (true) {
        // 若当前无设备，等待后重扫
        if (pfds.empty()) {
            usleep(300 * 1000);
            closeInputFds(inputFds);
            inputFds = scanAndOpenKeyDevices();
            pfds = buildPollFds(inputFds);
            continue;
        }

        int ret = poll(pfds.data(), static_cast<nfds_t>(pfds.size()), kPollTimeoutMs);
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("poll 失败，退出事件循环");
            break;
        }

        bool needRescan = false;

        if (ret == 0) {
            // 超时：尝试轻量重扫，适配热插拔/枚举变化
            std::vector<int> newFds = scanAndOpenKeyDevices();
            // 若集合不同则替换
            if (newFds.size() != inputFds.size()) {
                needRescan = true;
            } else {
                // 粗略比较（顺序可能不同，但代价可接受）
                for (size_t i = 0; i < newFds.size(); ++i) {
                    if (newFds[i] != inputFds[i]) { needRescan = true; break; }
                }
            }
            closeInputFds(newFds); // 先关闭扫描过程中的临时 fd
        }

        for (size_t i = 0; i < pfds.size(); ++i) {
            struct pollfd& p = pfds[i];
            if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                LOG_WARN("输入设备出现错误，fd=" + std::to_string(p.fd));
                needRescan = true;
                continue;
            }
            if (!(p.revents & POLLIN)) {
                continue;
            }

            // 读取并处理所有可用事件
            struct input_event ev{};
            ssize_t n = 0;
            while ((n = read(p.fd, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
                if (ev.type == EV_KEY && ev.code == KEY_F3) {
                    // 捕获 KEY_F3：读取 tri_state（失败一次性重开重试）
                    bool ok = false;
                    if (triFd < 0) {
                        triFd = open("/proc/tristatekey/tri_state", O_RDONLY | O_CLOEXEC);
                    }
                    std::string curr;
                    if (triFd >= 0) {
                        curr = readTriStateFromFd(triFd, ok);
                    }
                    if (!ok) {
                        if (triFd >= 0) { close(triFd); triFd = -1; }
                        triFd = open("/proc/tristatekey/tri_state", O_RDONLY | O_CLOEXEC);
                        if (triFd >= 0) {
                            curr = readTriStateFromFd(triFd, ok);
                        }
                    }
                    if (!ok) {
                        LOG_ERROR("读取 /proc/tristatekey/tri_state 失败，退出事件循环");
                        shouldExit = true;
                        break;
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
                LOG_WARN("读取输入设备失败，fd=" + std::to_string(p.fd));
                needRescan = true;
            }
            p.revents = 0;

            if (shouldExit) break;
        }

        if (shouldExit) {
            break;
        }

        if (needRescan) {
            closeInputFds(inputFds);
            inputFds = scanAndOpenKeyDevices();
            pfds = buildPollFds(inputFds);
        }
    }

    // 清理：关闭 fd
    closeInputFds(inputFds);
    if (triFd >= 0) close(triFd);
    LOG_INFO("三段键事件监听已退出");
}

void threeStageExit() {
    // no-op：退出时机由线程/进程销毁或致命 I/O 错误决定
}