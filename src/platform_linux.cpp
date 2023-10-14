#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "platform.h"

static struct Context_Linux
{
	SDL_Window *window;
	SDL_GLContext gl_context;

	bool has_dropped_file = false;
	std::filesystem::path dropped_file;
} ctx;

bool platform_create_window()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags const window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    ctx.window = SDL_CreateWindow("Fantasy Quantum Computer",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  1280, 800,
                                  window_flags);

    ctx.gl_context = SDL_GL_CreateContext(ctx.window);
    SDL_GL_MakeCurrent(ctx.window, ctx.gl_context);
    SDL_GL_SetSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGui_ImplSDL2_InitForOpenGL(ctx.window, ctx.gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

	return true;
}

void platform_destroy_window()
{
	ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(ctx.gl_context);
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();
}

bool platform_update()
{
	bool done = false;
	SDL_Event event;
	while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            done = true;
        }
        else if (event.type == SDL_WINDOWEVENT &&
                 event.window.event == SDL_WINDOWEVENT_CLOSE &&
                 event.window.windowID == SDL_GetWindowID(ctx.window)) {
            done = true;
	    }
	    else if (event.type == SDL_DROPFILE) {
	    	ctx.has_dropped_file = true;
            ctx.dropped_file = event.drop.file;
            SDL_free(event.drop.file);
        }
    }

	if (done) {
		return true;
	}

	ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

	return false;
}

void platform_render()
{
	static float const clear_color[4] = { 0.45f, 0.55f, 0.60f, 1.00f };

	ImGui::Render();
	glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
	glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	SDL_GL_SwapWindow(ctx.window);
}

void platform_quit()
{
	SDL_Event quit_event = { };
	quit_event.type = SDL_QUIT;
	SDL_PushEvent(&quit_event);
}

std::optional<std::filesystem::path> platform_get_dropped_file()
{
	if (ctx.has_dropped_file) {
		ctx.has_dropped_file = false;
		return ctx.dropped_file;
	} else {
		return std::optional<std::filesystem::path>();
	}
}
