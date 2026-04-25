#ifndef NIKONTI2PLUGIN_H
#define NIKONTI2PLUGIN_H

#include <filesystem>

void InitPlugin(const std::filesystem::path& configDirPath);
void ShutdownPlugin();

#endif // NIKONTI2PLUGIN_H