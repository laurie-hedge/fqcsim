#include <cmath>
#include <fstream>

#include "constants.h"
#include "imgui_internal.h"
#include "implot.h"
#include "platform.h"
#include "qasm.h"
#include "qsim_gui.h"
#include "version.h"

static std::string to_binary_string(uint32_t state)
{
	char binary_string[NUM_QBITS + 1];
	for (uint8_t shift = 0; shift < NUM_QBITS; ++shift) {
		binary_string[shift] = (state & (1 << shift)) ? '1' : '0';
	}
	binary_string[NUM_QBITS] = '\0';
	return std::string(binary_string);
}

static std::string to_complex_string(std::complex<double> number)
{
	return std::to_string(number.real()) + "+" + std::to_string(number.imag()) + "i";
}

QSim_GUI::QSim_GUI(QSim *qsim) :
	qsim(qsim),
	num_runs(100)
{
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	float const x_axis_length = 4.0f * CONST_PI_F;
	float const x_axis_start = x_axis_length * -0.5f;
	float const step = x_axis_length / (float)samples_x.size();
	for (size_t index = 0; index < samples_x.size(); ++index) {
		samples_x[index] = x_axis_start + (step * (float)index);
	}

	update_waveform_samples();
}

QSim_GUI::~QSim_GUI()
{
	ImGui::PopFont();
	delete program;
}

void QSim_GUI::update()
{
	std::vector<Amplitude> amplitudes = qsim->get_amplitudes();

	update_main_window();
	update_program_window();
	update_console_window();
	update_control_window();
	update_state_window(amplitudes);
	update_results_window();
	update_probabilities_window(amplitudes);
	update_waveform_window();

	process_shortcuts();
}

void QSim_GUI::reload_program()
{
	load_source_file(program_source_file);
	update_waveform_samples();
}

void QSim_GUI::handle_file_drop(std::filesystem::path const &file)
{
	load_source_file(file);
	update_waveform_samples();
}

void QSim_GUI::update_main_window()
{
	static ImGuiWindowFlags const window_flags = ImGuiWindowFlags_MenuBar |
												 ImGuiWindowFlags_NoDocking |
												 ImGuiWindowFlags_NoTitleBar |
												 ImGuiWindowFlags_NoCollapse |
												 ImGuiWindowFlags_NoResize |
												 ImGuiWindowFlags_NoMove |
												 ImGuiWindowFlags_NoBringToFrontOnFocus |
												 ImGuiWindowFlags_NoNavFocus |
												 ImGuiWindowFlags_NoBackground;

	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("Main", nullptr, window_flags);

	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2);

	ImGuiID dockspace_id = ImGui::GetID("DockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	first_time_setup(dockspace_id, viewport->Size);

	update_menu_bar();

	ImGui::End();
}

void QSim_GUI::update_program_window()
{
	ImGui::Begin("Program");
	if (program) {
		static float const column_width = 70.0f;
		static float const row_height = 50.0f;
		static float const box_size = 40.0f;
		static float const cnot_control_radius = box_size * 0.2f;
		static float const cnot_target_radius = cnot_control_radius * 2.0f;
		static float const swap_x_size = box_size * 0.4f;

		std::vector<uint8_t> const &active_qbits = program->get_active_qbits();
		std::vector<Operation> const &operations = program->get_operations();
		float const total_width = active_qbits.size() * column_width;
		float const total_height = (operations.size() + 1) * row_height;

		ImGui::BeginChild("Child", ImVec2(total_width, total_height), false, ImGuiWindowFlags_HorizontalScrollbar);
		ImDrawList *draw_list = ImGui::GetWindowDrawList();
		ImVec2 const origin = ImGui::GetCursorScreenPos();
		ImGui::SetWindowFontScale(2.0f);

		for (size_t i = 0; i < active_qbits.size(); ++i) {
			ImVec2 const column_top_left { origin.x + (i * column_width), origin.y };
			std::string const label = "q" + std::to_string(active_qbits[i]);
			ImVec2 const text_size = ImGui::CalcTextSize(label.c_str());
			ImVec2 const label_pos { column_top_left.x + (column_width * 0.5f) - (text_size.x * 0.5f),
									 column_top_left.y + (text_size.y * 0.5f) };
			draw_list->AddText(label_pos, IM_COL32_WHITE, label.c_str());

			ImVec2 const line_start { label_pos.x + (text_size.x * 0.5f), origin.y + row_height };
			ImVec2 const line_end { line_start.x, origin.y + total_height };
			draw_list->AddLine(line_start, line_end, IM_COL32(140, 140, 140, 255), 3.0f);
		}

		auto draw_box_gate = [&](char const *gate, uint8_t qbit, size_t row_index) {
			size_t const column_index = std::find(active_qbits.begin(), active_qbits.end(), qbit) - active_qbits.begin();
			ImVec2 const box_top_left {
				origin.x + (column_index * column_width) + ((column_width - box_size) * 0.5f),
				origin.y + (row_index * row_height) + ((row_height - box_size) * 0.5f)
			};
			ImVec2 const box_bottom_right {
				box_top_left.x + box_size,
				box_top_left.y + box_size
			};
			draw_list->AddRectFilled(box_top_left, box_bottom_right, IM_COL32_WHITE);

			ImVec2 const text_size = ImGui::CalcTextSize(gate);
			ImVec2 const label_pos { box_top_left.x + (box_size * 0.5f) - (text_size.x * 0.5f),
									 box_top_left.y + (text_size.y * 0.25f) };
			draw_list->AddText(label_pos, IM_COL32_BLACK, gate);
		};

		auto draw_cnot_gate = [&](uint8_t control_qbit, uint8_t target_qbit, size_t row_index) {
			size_t const control_column_index = std::find(active_qbits.begin(), active_qbits.end(), control_qbit) - active_qbits.begin();
			size_t const target_column_index = std::find(active_qbits.begin(), active_qbits.end(), target_qbit) - active_qbits.begin();
			ImVec2 const line_start {
				origin.x + ((control_column_index + 0.5f) * column_width),
				origin.y + ((row_index + 0.5f) * row_height)
			};
			ImVec2 const line_end {
				origin.x + ((target_column_index + 0.5f) * column_width),
				line_start.y
			};
			draw_list->AddLine(line_start, line_end, IM_COL32_WHITE, 2.0f);
			draw_list->AddCircleFilled(line_start, cnot_control_radius, IM_COL32_WHITE);
			draw_list->AddCircleFilled(line_end, cnot_target_radius, IM_COL32_WHITE);

			ImVec2 const text_size = ImGui::CalcTextSize("+");
			ImVec2 const label_pos { line_end.x - (text_size.x * 0.5f),
									 line_end.y - cnot_target_radius + (text_size.y * 0.1f) };
			draw_list->AddText(label_pos, IM_COL32_BLACK, "+");
		};

		auto draw_swap_gate = [&](uint8_t first_qbit, uint8_t second_qbit, size_t row_index) {
			size_t const control_column_index = std::find(active_qbits.begin(), active_qbits.end(), first_qbit) - active_qbits.begin();
			size_t const target_column_index = std::find(active_qbits.begin(), active_qbits.end(), second_qbit) - active_qbits.begin();
			ImVec2 const line_start {
				origin.x + ((control_column_index + 0.5f) * column_width),
				origin.y + ((row_index + 0.5f) * row_height)
			};
			ImVec2 const line_end {
				origin.x + ((target_column_index + 0.5f) * column_width),
				line_start.y
			};
			draw_list->AddLine(line_start, line_end, IM_COL32_WHITE, 2.0f);

			float const half_x_size = swap_x_size * 0.5f;
			draw_list->AddLine(
				{ line_start.x - half_x_size, line_start.y - half_x_size },
				{ line_start.x + half_x_size, line_start.y + half_x_size },
				IM_COL32_WHITE, 2.0f);
			draw_list->AddLine(
				{ line_start.x - half_x_size, line_start.y + half_x_size },
				{ line_start.x + half_x_size, line_start.y - half_x_size },
				IM_COL32_WHITE, 2.0f);

			draw_list->AddLine(
				{ line_end.x - half_x_size, line_end.y - half_x_size },
				{ line_end.x + half_x_size, line_end.y + half_x_size },
				IM_COL32_WHITE, 2.0f);
			draw_list->AddLine(
				{ line_end.x - half_x_size, line_end.y + half_x_size },
				{ line_end.x + half_x_size, line_end.y - half_x_size },
				IM_COL32_WHITE, 2.0f);
		};

		auto draw_toffoli_gate = [&](uint8_t first_control_qbit, uint8_t second_control_qbit, uint8_t target_qbit, size_t row_index) {
			size_t const first_control_column_index = std::find(active_qbits.begin(), active_qbits.end(), first_control_qbit) - active_qbits.begin();
			size_t const second_control_column_index = std::find(active_qbits.begin(), active_qbits.end(), second_control_qbit) - active_qbits.begin();
			size_t const target_column_index = std::find(active_qbits.begin(), active_qbits.end(), target_qbit) - active_qbits.begin();

			ImVec2 const first_control_pos {
				origin.x + ((first_control_column_index + 0.5f) * column_width),
				origin.y + ((row_index + 0.5f) * row_height)
			};
			ImVec2 const second_control_pos {
				origin.x + ((second_control_column_index + 0.5f) * column_width),
				first_control_pos.y
			};
			ImVec2 const target_pos {
				origin.x + ((target_column_index + 0.5f) * column_width),
				first_control_pos.y
			};

			auto pos_comparison = [](ImVec2 lhs, ImVec2 rhs) { return lhs.x < rhs.x; };
			draw_list->AddLine(std::min({first_control_pos, second_control_pos, target_pos}, pos_comparison),
							   std::max({first_control_pos, second_control_pos, target_pos}, pos_comparison),
							   IM_COL32_WHITE, 2.0f);

			draw_list->AddCircleFilled(first_control_pos, cnot_control_radius, IM_COL32_WHITE);
			draw_list->AddCircleFilled(second_control_pos, cnot_control_radius, IM_COL32_WHITE);
			draw_list->AddCircleFilled(target_pos, cnot_target_radius, IM_COL32_WHITE);

			ImVec2 const text_size = ImGui::CalcTextSize("+");
			ImVec2 const label_pos { target_pos.x - (text_size.x * 0.5f),
									 target_pos.y - cnot_target_radius + (text_size.y * 0.1f) };
			draw_list->AddText(label_pos, IM_COL32_BLACK, "+");
		};

		size_t row_index = 1;
		for (auto const &operation : operations) {
			switch (operation.gate) {
				case Gate::CNOT: {
					draw_cnot_gate(operation.operands[0], operation.operands[1], row_index);
				} break;
				case Gate::IDENTITY: {
					draw_box_gate("I", operation.operands[0], row_index);
				} break;
				case Gate::HADAMARD: {
					draw_box_gate("H", operation.operands[0], row_index);
				} break;
				case Gate::PAULI_X: {
					draw_box_gate("X", operation.operands[0], row_index);
				} break;
				case Gate::PAULI_Y: {
					draw_box_gate("Y", operation.operands[0], row_index);
				} break;
				case Gate::PAULI_Z: {
					draw_box_gate("Z", operation.operands[0], row_index);
				} break;
				case Gate::R_X: {
					draw_box_gate("Rx", operation.operands[0], row_index);
				} break;
				case Gate::R_Y: {
					draw_box_gate("Ry", operation.operands[0], row_index);
				} break;
				case Gate::R_Z: {
					draw_box_gate("Rz", operation.operands[0], row_index);
				} break;
				case Gate::S: {
					draw_box_gate("S", operation.operands[0], row_index);
				} break;
				case Gate::SWAP: {
					draw_swap_gate(operation.operands[0], operation.operands[1], row_index);
				} break;
				case Gate::S_DAG: {
					draw_box_gate("S'", operation.operands[0], row_index);
				} break;
				case Gate::T: {
					draw_box_gate("T", operation.operands[0], row_index);
				} break;
				case Gate::T_DAG: {
					draw_box_gate("T'", operation.operands[0], row_index);
				} break;
				case Gate::TOFFOLI: {
					draw_toffoli_gate(operation.operands[0], operation.operands[1], operation.operands[2], row_index);
				} break;
			}
			row_index += 1;
		}

		size_t const next_gate = qsim->get_next_gate_index();
		ImVec2 const line_start { origin.x, origin.y + ((next_gate + 1) * row_height) - 1.0f };
		ImVec2 const line_end { origin.x + total_width, line_start.y };
		draw_list->AddLine(line_start, line_end, IM_COL32(255, 0, 0, 255), 2.0f);

		ImGui::SetWindowFontScale(1.0f);
		ImGui::EndChild();
	}
	ImGui::End();
}

void QSim_GUI::update_console_window()
{
	ImGui::Begin("Console");
	ImGui::InputTextMultiline("Console", const_cast<char *>(console_text.c_str()), console_text.size(), ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly);
	ImGui::End();
}

void QSim_GUI::update_control_window()
{
	ImGui::Begin("Controls");
	if (ImGui::Button("Reset")) {
		handle_reset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Run")) {
		handle_run();
	}
	ImGui::SameLine();
	if (ImGui::Button("Step")) {
		handle_step();
	}
	ImGui::InputInt("Number of iterations", &num_runs);
	if (num_runs < 1) {
		num_runs = 1;
	} else if (num_runs > 10000) {
		num_runs = 10000;
	}
	ImGui::End();
}

void QSim_GUI::update_state_window(std::vector<Amplitude> const &amplitudes)
{
	ImGui::Begin("State");
	for (auto const &amplitude : amplitudes) {
		std::string const state_name = "|" + to_binary_string(amplitude.state) + ">";
		if (ImGui::TreeNode(state_name.c_str())) {
			std::string const amplitude_string = "Amplitude: " + to_complex_string(amplitude.amplitude);
			ImGui::TextUnformatted(amplitude_string.c_str());
			std::complex<double> const probability = std::pow(amplitude.amplitude, 2);
			double const percentage = std::abs(probability) * 100.0;
			std::string const probability_string = "Probability: " + std::to_string(percentage) + "%";
			ImGui::TextUnformatted(probability_string.c_str());
			ImGui::TreePop();
		}
	}
	ImGui::End();
}

void QSim_GUI::update_results_window()
{
	ImGui::Begin("Results");
	std::vector<Result> const &results = qsim->get_results();
	if (!results.empty()) {
		static char const * const shots_label = "Shots";
		size_t const num_results = results.size();
		uint32_t *data = new uint32_t[num_results];
		double *positions = new double[num_results];
		char const **column_labels = new char const *[num_results];

		std::vector<std::string> column_label_strings;

		for (size_t i = 0; i < num_results; ++i) {
			data[i] = results[i].num_times;
			positions[i] = (double)i;
			column_label_strings.push_back("|" + to_binary_string(results[i].state) + ">");
		}

		for (size_t i = 0; i < num_results; ++i) {
			column_labels[i] = column_label_strings[i].c_str();
		}

		if (ImPlot::BeginPlot("Results", { -1, -1 })) {
			ImPlot::SetupAxes("State", "Occurrences", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
			ImPlot::SetupAxisTicks(ImAxis_X1, positions, (int)num_results, column_labels);
			ImPlot::PlotBarGroups(&shots_label, data, 1, (int)num_results);
			ImPlot::EndPlot();
		}

		delete[] column_labels;
		delete[] positions;
		delete[] data;
	} else {
		ImGui::Text("Run program to generate results");
	}
	ImGui::End();
}

void QSim_GUI::update_probabilities_window(std::vector<Amplitude> const &amplitudes)
{
	ImGui::Begin("Probabilities");

	std::vector<std::string> segment_label_strings;
	char const **segment_labels = new char const *[amplitudes.size()];
	float *data = new float[amplitudes.size()];

	for (size_t index = 0; index < amplitudes.size(); ++index) {
		segment_label_strings.push_back("|" + to_binary_string(amplitudes[index].state) + ">");
	}
	for (size_t index = 0; index < amplitudes.size(); ++index) {
		segment_labels[index] = segment_label_strings[index].c_str();
		data[index] = (float)std::abs(std::pow(amplitudes[index].amplitude, 2));
	}

	if (ImPlot::BeginPlot("Probabilities", { -1, -1 }, ImPlotFlags_Equal | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs )) {
		ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
		ImPlot::SetupAxesLimits(-0.5, 0.5, -0.5, 0.5);
		ImPlot::PlotPieChart(segment_labels, data, (int)amplitudes.size(), 0.0, 0.0, 0.2, "%.2f");
		ImPlot::EndPlot();
	}

	delete[] segment_labels;
	delete[] data;

	ImGui::End();
}

void QSim_GUI::update_waveform_window()
{
	ImGui::Begin("Waveform");

	if (ImPlot::BeginPlot("Waveform", { -1, -1 }, ImPlotFlags_NoInputs)) {
		ImPlot::SetupAxesLimits(samples_x[0], samples_x[samples_x.size() - 1], -2.0, 2.0);
		for (size_t qbit_index = 0; qbit_index < NUM_QBITS; ++qbit_index) {
			std::string label = "q" + std::to_string(qbit_index);
			ImPlot::PlotLine(label.c_str(), samples_x.data(), samples_y[qbit_index].data(), (int)num_samples);
		}
		ImPlot::EndPlot();
	}

	ImGui::End();
}

void QSim_GUI::first_time_setup(ImGuiID dockspace_id, ImVec2 size)
{
	if (first_time) {
		first_time = false;

		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, size);

		ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id);
		ImGuiID dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Console", dock_id_down);
		ImGui::DockBuilderDockWindow("Controls", dock_id_down);
		ImGui::DockBuilderDockWindow("State", dock_id_down);
		ImGui::DockBuilderDockWindow("Program", dock_id_left);
		ImGui::DockBuilderDockWindow("Results", dockspace_id);
		ImGui::DockBuilderDockWindow("Probabilities", dockspace_id);
		ImGui::DockBuilderDockWindow("Waveform", dockspace_id);
		ImGui::DockBuilderFinish(dockspace_id);
	}
}

void QSim_GUI::update_menu_bar()
{
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Main")) {
			if (ImGui::MenuItem("Load Program", "Ctrl+O")) {
				handle_load();
			}
			if (ImGui::MenuItem("Save Results", "Ctrl+S", false, !qsim->get_results().empty())) {
				handle_save();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
				handle_quit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Sim")) {
			if (ImGui::MenuItem("Reset", "R")) {
				handle_reset();
			}
			if (ImGui::MenuItem("Run", "F5")) {
				handle_run();
			}
			if (ImGui::MenuItem("Step", "Space")) {
				handle_step();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			ImGui::MenuItem("Fantasy Quantum Computer v" VERSION, nullptr, false, false);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void QSim_GUI::process_shortcuts()
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.KeyCtrl) {
		if (ImGui::IsKeyPressed(ImGuiKey_O)) {
			handle_load();
		} else if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
			handle_quit();
		} else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
			if (!qsim->get_results().empty()) {
				handle_save();
			}
		}
	} else {
		if (ImGui::IsKeyPressed(ImGuiKey_R)) {
			handle_reset();
		} else if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
			handle_run();
		} else if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
			handle_step();
		}
	}
}

void QSim_GUI::handle_load()
{
	std::filesystem::path source_file = platform_open_file_dialog();
	if (!source_file.empty()) {
		load_source_file(source_file);
		update_waveform_samples();
	}
}

void QSim_GUI::handle_save()
{
	std::filesystem::path write_file = platform_save_file_dialog();
	if (!write_file.empty()) {
		save_results_file(write_file);
	}
}

void QSim_GUI::handle_reset()
{
	qsim->reset();
	update_waveform_samples();
}

void QSim_GUI::handle_run()
{
	qsim->run(num_runs);
	update_waveform_samples();
}

void QSim_GUI::handle_step()
{
	qsim->step();
	update_waveform_samples();
}

void QSim_GUI::handle_quit()
{
	platform_quit();
}

void QSim_GUI::load_source_file(std::filesystem::path const &source_file)
{
	if (program) {
		delete program;
		program = nullptr;
	}

	bool const is_reload = source_file == program_source_file;

	std::string const message = std::string(is_reload ? "Reloading" : "Loading") + " file " + source_file.string() + "...";
	print_to_console(message);

	std::ifstream file_stream { source_file };
	if (file_stream.is_open()) {
		std::string source_code { std::istreambuf_iterator<char>(file_stream),
								  std::istreambuf_iterator<char>() };
		program = new Quantum_Program(source_code);
		if (program->is_valid()) {
			print_to_console("Build successful");
		} else {
			print_to_console("Failed to compile " + source_file.string() + ".\nError: " + program->get_build_error());
			delete program;
			program = nullptr;
		}
	} else {
		print_to_console("Failed to load file " + source_file.string());
	}

	qsim->set_program(program);

	if (!is_reload) {
		program_source_file = source_file;
		platform_set_file_change_notifications(program_source_file);
	}
}

void QSim_GUI::save_results_file(std::filesystem::path const &results_file)
{
	std::ofstream file_stream { results_file };
	if (file_stream.is_open()) {
		file_stream << "state,occurrences\n";
		auto const &results = qsim->get_results();
		for (auto const &result : results) {
			file_stream << '|' << to_binary_string(result.state) << ">," << result.num_times << '\n';
		}
		print_to_console("Saved results to " + results_file.string());
	} else {
		print_to_console("Failed to save file " + results_file.string());
	}
}

void QSim_GUI::print_to_console(std::string const &message)
{
	console_text += message + '\n';
}

void QSim_GUI::update_waveform_samples()
{
	static double const ket_zero_freq_mul = 1.0;
	static double const ket_one_freq_mul = 2.0;
	for (size_t qbit_index = 0; qbit_index < samples_y.size(); ++qbit_index) {
		std::array<std::complex<double>, 2> const qbit_state = qsim->get_qbit_state((uint8_t)qbit_index);
		for (size_t sample_index = 0; sample_index < samples_y[qbit_index].size(); ++sample_index) {
			float const zero_amplitude = (float)(std::sin((samples_x[sample_index] + (qbit_state[0].imag() * CONST_TAU)) * ket_zero_freq_mul) * std::abs(qbit_state[0]));
			float const one_amplitude = (float)(std::sin((samples_x[sample_index] + (qbit_state[1].imag() * CONST_TAU)) * ket_one_freq_mul) * std::abs(qbit_state[1]));
			samples_y[qbit_index][sample_index] = zero_amplitude + one_amplitude;
		}
	}
}
