#include "three-stage_main.h"
#include "logger.h"
#include <string>
#include <unistd.h>
#include "utils.h"
#include <poll.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>

static std::string readTriStateFromFd(int fd, bool& ok) {
    if (fd < 0) {
        ok = false;
        return {};
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        ok = false;
        return {};
    }

    char buf[16];
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

void threeStageMain(const std::string& mod_dir) {
    // 仅打开固定设备 /dev/input/event6
    int fd = open("/dev/input/event6", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        LOG_ERROR("无法打开 /dev/input/event6，程序退出");
        return;
    }

    struct pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    // tri_state fd 在事件循环生命周期内持久化
    int triFd = open("/proc/tristatekey/tri_state", O_RDONLY | O_CLOEXEC);
    if (triFd < 0) {
        LOG_WARN("无法打开 /proc/tristatekey/tri_state，启动后将于首次 F3 尝试重开");
    }

    // 初始化 tri_state（若失败，等待首次 KEY_F3 再初始化，不触发脚本）
    std::string lastValue;
    bool hasInitial = false;
    {
        bool ok = false;
        if (triFd >= 0) {
            std::string initVal = readTriStateFromFd(triFd, ok);
            if (ok) {
                lastValue = initVal;
                hasInitial = true;
                LOG_INFO(std::string("三段键初始值: ") + initVal);
            } else {
                LOG_WARN("无法读取 /proc/tristatekey/tri_state，等待事件后再初始化");
            }
        } else {
            LOG_WARN("无法读取 /proc/tristatekey/tri_state，等待事件后再初始化");
        }
    }

    // 事件循环：等待输入事件，仅在 KEY_F3 时读取 tri_state 并按变化触发脚本
    bool shouldExit = false;
    while (true) {
        int ret = poll(&pfd, static_cast<nfds_t>(1), -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("poll 失败，退出事件循环");
            break;
        }
        if (ret == 0) {
            // 超时：仅用于检查退出标志
            continue;
        }

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            LOG_WARN("输入设备出现错误，fd=" + std::to_string(pfd.fd));
            break;
        }
        if (!(pfd.revents & POLLIN)) {
            pfd.revents = 0;
            continue;
        }

        // 读取并处理所有可用事件
        struct input_event ev{};
        ssize_t n = 0;
        while ((n = read(pfd.fd, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
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
                    // 事件驱动下的首次初始化，不触发脚本
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
            shouldExit = true;
        }
        pfd.revents = 0;

        if (shouldExit) {
            break;
        }
    }

    // 清理：关闭 fd
    if (pfd.fd >= 0) close(pfd.fd);
    if (triFd >= 0) close(triFd);
    LOG_INFO("三段键事件监听已退出");
}

void threeStageExit() {
    // no-op：退出时机由线程/进程销毁或致命 I/O 错误决定
}