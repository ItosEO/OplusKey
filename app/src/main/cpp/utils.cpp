#include "utils.h"
#include <unistd.h>
#include <sys/types.h>
#include <csignal>
#include <vector>
#include <string>

void execBackground(const std::string& file, const std::vector<std::string>& args) {
    pid_t pid = fork();
    if (pid < 0) {
        // fork 失败
        return;
    }

    if (pid == 0) {
        // 子进程
        setsid(); // 脱离控制终端，防止挂起

        // 关闭标准输入输出
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        // 构造 argv
        std::vector<char*> argv;
        argv.reserve(args.size() + 2);
        argv.push_back(const_cast<char*>(file.c_str())); // argv[0]
        for (auto& s : args) {
            argv.push_back(const_cast<char*>(s.c_str()));
        }
        argv.push_back(nullptr);

        // 执行
        execvp(file.c_str(), argv.data());

        // execvp 失败才会执行到这里
        _exit(127);
    } else {
        // 父进程：不等待，直接返回
        signal(SIGCHLD, SIG_IGN); // 避免子进程成为僵尸
    }
}