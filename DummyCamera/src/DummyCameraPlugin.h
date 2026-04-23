#ifndef DUMMYCAMERAPLUGIN_H
#define DUMMYCAMERAPLUGIN_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

void InitPlugin(const std::filesystem::path& configDirPath);

void ShutdownPlugin();

#endif // DUMMYCAMERAPLUGIN_H