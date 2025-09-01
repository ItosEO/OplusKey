#include "three-stage_main.h"
#include "logger.h"
#include <fstream>
#include <string>
#include <atomic>
#include <unistd.h>
#include "utils.h"

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

void threeStageMain(const std::string& mod_dir) {
    g_running = true;
    LOG_INFO("开始监听 /proc/tristatekey/tri_state");

    std::string lastValue;
    bool hasInitial = false;

    while (g_running) {
        bool ok = false;
        std::string curr = readTriStateOnce(ok);
        if (!ok) {
            LOG_ERROR("无法打开 /proc/tristatekey/tri_state，1000ms 后重试");
            usleep(1000 * 1000); // 1000ms 重试
            continue;
        }

        if (!hasInitial) {
            LOG_INFO(std::string("三段键初始值: ") + curr);
            lastValue = curr;
            hasInitial = true;
        } else if (curr != lastValue) {
            LOG_INFO(std::string("三段键状态变化: [") + lastValue + "] -> [" + curr + "]");
            lastValue = curr;

            // 映射 tri_state 到参数，并执行脚本（仅在变化时）
            std::string arg;
            if (curr == "1") arg = "up";
            else if (curr == "2") arg = "center";
            else if (curr == "3") arg = "down";

            if (!arg.empty()) {
                LOG_INFO(std::string("执行 t-stage.sh，参数: ") + arg);
                execBackground(DEFAULT_SHELL, {mod_dir + "/t-stage.sh", arg});
            } else {
                LOG_WARN(std::string("未知的三段键值，跳过执行: ") + curr);
            }
        }

        usleep(500 * 1000); // 500ms 轮询间隔
    }
}

void threeStageExit() {
    g_running = false;
}