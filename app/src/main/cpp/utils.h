#ifndef OPLUSKEY_UTILS_H
#define OPLUSKEY_UTILS_H

#include <string>
#include <vector>

#define DEFAULT_SHELL "/system/bin/sh"

void execBackground(const std::string& file, const std::vector<std::string>& args);

#endif //OPLUSKEY_UTILS_H
