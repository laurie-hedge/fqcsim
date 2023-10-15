#pragma once

#include <array>
#include <string>
#include <filesystem>

#include "constants.h"
#include "imgui.h"
#include "ImGuiFileBrowser.h"
#include "qsim.h"

class QSim;
class Quantum_Program;

class QSim_GUI
{
	bool first_time = true;
	QSim *qsim;
	Quantum_Program *program = nullptr;
	std::string console_text;
	std::filesystem::path program_source_file;
	std::filesystem::file_time_type last_file_write_time;
	int num_runs;

	static constexpr size_t num_samples = 512;
	std::array<float, num_samples> samples_x;
	std::array<std::array<float, num_samples>, NUM_QBITS> samples_y;

	imgui_addons::ImGuiFileBrowser file_dialog;
	bool open_load = false;
	bool open_save = false;

	ImFont *font_normal, *font_large;

public:
	QSim_GUI(QSim *qsim);
	~QSim_GUI();

	void update();

	void handle_file_drop(std::filesystem::path const &file);

private:
	void update_main_window();
	void update_program_window();
	void update_console_window();
	void update_control_window();
	void update_state_window(std::vector<Amplitude> const &amplitudes);
	void update_results_window();
	void update_probabilities_window(std::vector<Amplitude> const &amplitudes);
	void update_waveform_window();

	void first_time_setup(ImGuiID dockspace_id, ImVec2 size);
	void update_menu_bar();
	void update_dialogs();

	void process_shortcuts();

	void handle_load();
	void handle_save();
	void handle_reset();
	void handle_run();
	void handle_step();
	void handle_quit();

	void load_source_file(std::filesystem::path const &source_file);
	void save_results_file(std::filesystem::path const &results_file);

	bool source_file_has_changed() const;
	void reload_program();

	void print_to_console(std::string const &message);

	void update_waveform_samples();
};
