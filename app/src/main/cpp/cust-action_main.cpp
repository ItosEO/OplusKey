#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#include <filesystem>
#include "cust-action_main.h"
#include "logger.h"
#include "utils.h"

long lastClickTime = 0;

void onClick(long usedTime, long currentTime, const std::string &mod_dir) {
    if (usedTime < 500) { // 短按
        // 是否使用双击
        if (std::filesystem::exists(mod_dir + "/double_click")) {
            if (currentTime - lastClickTime < 400) { // 双击
                LOG_INFO("Double Click detected");
                lastClickTime = 0; // 重置，防止三击被误判为双击
                // 后台执行shell文件
                execBackground(DEFAULT_SHELL, {mod_dir + "/cust.sh", "false"});
            } else {
                lastClickTime = currentTime;
            }
        } else {
            // 单击
            LOG_INFO("Single Click detected");
            // 后台执行shell文件
            execBackground(DEFAULT_SHELL, {mod_dir + "/cust.sh", "false"});
        }
    } else { // 长按
        LOG_INFO("Long Press detected");
        lastClickTime = 0; // 重置，防止长按后紧接着的单击被误判为双击
        // 后台执行shell文件
        execBackground(DEFAULT_SHELL, {mod_dir + "/cust.sh", "true"});
    }
}

int fd = -1;

void custActionMain(const std::string &mod_dir) {
    // 打开自定义按键对应的设备描述文件
    const char *device = "/dev/input/event0";
    fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("Unable to open device");
        exit(1);
    }

    // 持续监听
    struct input_event ev{};
    long pressTime = 0;
    while (true) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == (ssize_t) sizeof(ev)) {
            if (ev.code == 0x02df && ev.type == 1) {
                LOG_INFO("Customizable action button event: value=" + std::to_string(ev.value) +
                         ", time=" +
                         std::to_string(ev.time.tv_sec) + "." + std::to_string(ev.time.tv_usec));
                if (ev.value == 1) { // 按下
                    pressTime = ev.time.tv_sec * 1000 + ev.time.tv_usec / 1000;
                } else if (ev.value == 0) { // 抬起
                    long currentTime = ev.time.tv_sec * 1000 + ev.time.tv_usec / 1000;
                    long usedTime = currentTime - pressTime;
                    onClick(usedTime, currentTime, mod_dir);
                }
            }
        } else {
            perror("Read error");
            break;
        }
    }

    close(fd);
}

void custActionExit() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}