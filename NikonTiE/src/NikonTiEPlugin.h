#ifndef NIKONTIEPLUGIN_H
#define NIKONTIEPLUGIN_H

#include <filesystem>

void InitPlugin(const std::filesystem::path& configDirPath);
void ShutdownPlugin();

#endif // NIKONTIEPLUGIN_H
