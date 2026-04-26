#ifndef OLYMPUSIX83PLUGIN_H
#define OLYMPUSIX83PLUGIN_H

#include <filesystem>

void InitPlugin(const std::filesystem::path& configDirPath);
void ShutdownPlugin();

#endif // OLYMPUSIX83PLUGIN_H