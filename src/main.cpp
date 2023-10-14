#include "platform.h"
#include "qsim.h"
#include "qsim_gui.h"

int main(int argc, char const **argv)
{
	if (!platform_create_window()) {
		return 1;
	}

	QSim sim;
	QSim_GUI gui(&sim);

	if (argc > 1) {
		gui.handle_file_drop(std::filesystem::path(argv[1]));
	}

	while (!platform_update()) {
		auto dropped_file = platform_get_dropped_file();
		if (dropped_file) {
			gui.handle_file_drop(*dropped_file);
		}

		gui.update();
		platform_render();
	}

	platform_destroy_window();

	return 0;
}
