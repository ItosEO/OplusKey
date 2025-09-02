#include "three-stage_main.h"
#include "logger.h"
#include <fstream>
#include <string>
#include <atomic>
#include <unistd.h>
#include "utils.h"
// 新增头文件：事件与设备
#include <vector>
#include <poll.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>

static std::atomic<bool> g_running{false};

static std::string readTriStateOnce(bool& ok) {
    std::ifstream fin("/proc/tristatekey/tri_state");
    if (!fin.is_open()) {
        ok = false;
        return {};
    }
    std::string line;
    std::getline(fin, line);
    ok = true;
    return line;
}

// 新增：打开输入设备 event0..event31（非阻塞）
static std::vector<int> openInputDevices() {
    std::vector<int> fds;
    fds.reserve(32);
    for (int i = 0; i <= 31; ++i) {
        std::string path = std::string("/dev/input/event") + std::to_string(i);
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            fds.push_back(fd);
        }
    }
    return fds;
}

void threeStageMain(const std::string& mod_dir) {
    g_running = true;
    LOG_INFO("三段键：进入基于事件的监听模式（KEY_F3）");

    // 打开输入设备
    std::vector<int> fds;
    while (g_running) {
        fds = openInputDevices();
        if (!fds.empty()) break;
        LOG_WARN("未找到任何 /dev/input/event* 设备，1000ms 后重试");
        usleep(1000 * 1000);
    }
    if (!g_running) {
        // 被要求退出
        return;
    }
    LOG_INFO("已打开输入设备数量: " + std::to_string(fds.size()));

    // 构造 poll 列表
    std::vector<struct pollfd> pfds;
    pfds.reserve(fds.size());
    for (int fd : fds) {
        struct pollfd p{};
        p.fd = fd;
        p.events = POLLIN;
        p.revents = 0;
        pfds.push_back(p);
    }

    // 初始化 tri_state（若失败，等待首次 KEY_F3 再初始化，不触发脚本）
    std::string lastValue;
    bool hasInitial = false;
    {
        bool ok = false;
        std::string initVal = readTriStateOnce(ok);
        if (ok) {
            lastValue = initVal;
            hasInitial = true;
            LOG_INFO(std::string("三段键初始值: ") + initVal);
        } else {
            LOG_WARN("无法读取 /proc/tristatekey/tri_state，等待事件后再初始化");
        }
    }

    // 事件循环：等待输入事件，仅在 KEY_F3 时读取 tri_state 并按变化触发脚本
    while (g_running) {
        int ret = poll(pfds.data(), static_cast<nfds_t>(pfds.size()), 3000);
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("poll 失败，退出事件循环");
            break;
        }
        if (ret == 0) {
            // 超时：仅用于检查退出标志
            continue;
        }

        for (size_t i = 0; i < pfds.size(); ++i) {
            auto &p = pfds[i];
            if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                LOG_WARN("输入设备出现错误，fd=" + std::to_string(p.fd));
                p.revents = 0;
                continue;
            }
            if (!(p.revents & POLLIN)) {
                p.revents = 0;
                continue;
            }

            // 读取并处理所有可用事件
            struct input_event ev{};
            ssize_t n = 0;
            while ((n = read(p.fd, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
                if (ev.type == EV_KEY && ev.code == KEY_F3) {
                    // 捕获 KEY_F3：读取 tri_state
                    bool ok = false;
                    std::string curr = readTriStateOnce(ok);
                    if (!ok) {
                        LOG_ERROR("读取 /proc/tristatekey/tri_state 失败，跳过本次事件");
                        continue;
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
                LOG_WARN("读取输入设备失败，fd=" + std::to_string(p.fd));
            }
            p.revents = 0;
        }
    }

    // 清理：关闭所有 fd
    for (auto &p : pfds) {
        if (p.fd >= 0) close(p.fd);
    }
    LOG_INFO("三段键事件监听已退出");
}

void threeStageExit() {
    g_running = false;
}