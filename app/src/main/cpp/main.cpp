#include <string>
#include <csignal>
#include "three-stage_main.h"
#include "cust-action_main.h"
#include "logger.h"

int curr_mode = -1; // 0: three-stage, 1: cust-action

void onExit() {
    if (curr_mode == 0) {
        threeStageExit();
    } else if (curr_mode == 1) {
        custActionExit();
    }
}

// 捕获强制退出信号
void signalHandler(int sig) {
    exit(0);
}

int main(int argc, char** argv) {
    // 未指定模式
    if (argc <= 1) {
        return 1;
    }

    // 添加退出hook
    atexit(onExit);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 获取模块目录
    std::string program = argv[0];
    std::string mod_dir = program.substr(0, program.find_last_of('/'));

    // 判断模式
    std::string mode = argv[1];
    if (mode == "three-stage") {
        curr_mode = 0;
        threeStageMain(mod_dir);
    } else if (mode == "cust-action") {
        curr_mode = 1;
        custActionMain(mod_dir);
    } else {
        return 1;
    }
    return 0;
}