#pragma once

#include <filesystem>
#include <optional>

bool platform_create_window();
void platform_destroy_window();
bool platform_update();
void platform_render();
void platform_quit();
std::optional<std::filesystem::path> platform_get_dropped_file();
