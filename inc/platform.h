#pragma once

#include <filesystem>
#include <optional>

bool platform_create_window();
void platform_destroy_window();
bool platform_update();
void platform_render();
void platform_quit();
std::filesystem::path platform_open_file_dialog();
std::filesystem::path platform_save_file_dialog();
std::optional<std::filesystem::path> platform_get_dropped_file();
void platform_set_file_change_notifications(std::filesystem::path const &file);
bool platform_get_file_change_notification();
