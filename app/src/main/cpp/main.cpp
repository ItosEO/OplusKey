#include <string>
#include "three-stage_main.h"
#include "cust-action_main.h"

int main(int argc, char** argv) {
    if (argc <= 1) {
        return 1;
    }
    std::string program = argv[0];
    std::string mod_dir = program.substr(0, program.find_last_of('/'));
    std::string mode = argv[1];
    if (mode == "three-stage") {
        threeStageMain(mod_dir);
    } else if (mode == "cust-action") {
        custActionMain(mod_dir);
    } else {
        return 1;
    }
    return 0;
}