#include <d3d11.h>
#include <tchar.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "implot.h"
#include "platform.h"

static struct Context_Win32
{
	WNDCLASSEXW wc = { sizeof(wc) };
	HWND hwnd = nullptr;
	ID3D11Device *d3d_device = nullptr;
	ID3D11DeviceContext *d3d_device_context = nullptr;
	IDXGISwapChain *swap_chain = nullptr;
	UINT resize_width = 0;
	UINT resize_height = 0;
	ID3D11RenderTargetView *main_render_target_view = nullptr;

	bool has_dropped_file = false;
	std::filesystem::path dropped_file;

	std::filesystem::path monitoring_file;
	FILETIME last_file_write_time;
} ctx;

static void create_render_target()
{
	ID3D11Texture2D *back_buffer;
	ctx.swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	ctx.d3d_device->CreateRenderTargetView(back_buffer, nullptr, &ctx.main_render_target_view);
	back_buffer->Release();
}

static bool create_d3d_device(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT create_device_flags = 0;
	D3D_FEATURE_LEVEL feature_level;
	const D3D_FEATURE_LEVEL feature_level_array[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_level_array, 2, D3D11_SDK_VERSION, &sd, &ctx.swap_chain, &ctx.d3d_device, &feature_level, &ctx.d3d_device_context);
	if (res == DXGI_ERROR_UNSUPPORTED) {
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, create_device_flags, feature_level_array, 2, D3D11_SDK_VERSION, &sd, &ctx.swap_chain, &ctx.d3d_device, &feature_level, &ctx.d3d_device_context);
	}
	if (res != S_OK) {
		return false;
	}

	create_render_target();
	return true;
}

static void destroy_render_target()
{
	if (ctx.main_render_target_view) {
		ctx.main_render_target_view->Release();
		ctx.main_render_target_view = nullptr;
	}
}

static void destroy_d3d_device()
{
	destroy_render_target();
	if (ctx.swap_chain) {
		ctx.swap_chain->Release();
		ctx.swap_chain = nullptr;
	}
	if (ctx.d3d_device_context) {
		ctx.d3d_device_context->Release();
		ctx.d3d_device_context = nullptr;
	}
	if (ctx.d3d_device) {
		ctx.d3d_device->Release();
		ctx.d3d_device = nullptr;
	}
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI wnd_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(window, msg, wparam, lparam)) {
		return true;
	}

	switch (msg)
	{
		case WM_SIZE: {
			if (wparam == SIZE_MINIMIZED) {
				return 0;
			}
			ctx.resize_width = (UINT)LOWORD(lparam);
			ctx.resize_height = (UINT)HIWORD(lparam);
			return 0;
		}
		case WM_SYSCOMMAND: {
			if ((wparam & 0xfff0) == SC_KEYMENU) {
				return 0;
			}
			break;
		}
		case WM_DROPFILES: {
			WCHAR buffer[1024];
			if (DragQueryFileW((HDROP)wparam, 0, buffer, 1024)) {
				ctx.dropped_file = std::filesystem::path(buffer);
				ctx.has_dropped_file = true;
			}
			DragFinish((HDROP)wparam);
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProcW(window, msg, wparam, lparam);
}

bool platform_create_window()
{
	ctx.wc.style = CS_CLASSDC;
	ctx.wc.lpfnWndProc = wnd_proc;
	ctx.wc.hInstance = GetModuleHandle(nullptr);
	ctx.wc.lpszClassName = L"Fantasy Quantum Computer";
	RegisterClassExW(&ctx.wc);
	ctx.hwnd = CreateWindowExW(WS_EX_ACCEPTFILES,
							   ctx.wc.lpszClassName,
							   ctx.wc.lpszClassName,
							   WS_OVERLAPPEDWINDOW,
							   100, 100,
							   1280, 800,
							   nullptr,
							   nullptr,
							   ctx.wc.hInstance,
							   nullptr);

	if (!create_d3d_device(ctx.hwnd)) {
		destroy_d3d_device();
		UnregisterClassW(ctx.wc.lpszClassName, ctx.wc.hInstance);
		return false;
	}

	ShowWindow(ctx.hwnd, SW_SHOWDEFAULT);
	UpdateWindow(ctx.hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGui_ImplWin32_Init(ctx.hwnd);
	ImGui_ImplDX11_Init(ctx.d3d_device, ctx.d3d_device_context);

	return true;
}

void platform_destroy_window()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	destroy_d3d_device();
	DestroyWindow(ctx.hwnd);
	UnregisterClassW(ctx.wc.lpszClassName, ctx.wc.hInstance);
}

bool platform_update()
{
	bool done = false;
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT) {
			done = true;
		}
	}

	if (done) {
		return true;
	}

	if (ctx.resize_width != 0 && ctx.resize_height != 0) {
		destroy_render_target();
		ctx.swap_chain->ResizeBuffers(0, ctx.resize_width, ctx.resize_height, DXGI_FORMAT_UNKNOWN, 0);
		ctx.resize_width = ctx.resize_height = 0;
		create_render_target();
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	return false;
}

void platform_render()
{
	static float const clear_color[4] = { 0.45f, 0.55f, 0.60f, 1.00f };

	ImGui::Render();
	ctx.d3d_device_context->OMSetRenderTargets(1, &ctx.main_render_target_view, nullptr);
	ctx.d3d_device_context->ClearRenderTargetView(ctx.main_render_target_view, clear_color);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ctx.swap_chain->Present(1, 0);
}

void platform_quit()
{
	PostQuitMessage(0);
}

std::filesystem::path platform_open_file_dialog()
{
	WCHAR buffer[1024] = { '\0' };
	OPENFILENAMEW filename = { sizeof(OPENFILENAMEW) };
	filename.hwndOwner = ctx.hwnd;
	filename.lpstrFile = buffer;
	filename.nMaxFile = 1024;
	filename.lpstrFilter = L"Quantum Assembly (*.qasm)\0*.QASM\0All\0*.*\0";
	filename.nFilterIndex = 1;
	filename.lpstrTitle = L"Open Quantum Assembly File";
	filename.lpstrInitialDir = nullptr;
	filename.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	BOOL ok = GetOpenFileNameW(&filename);
	ImGui::GetIO().ClearInputKeys();
	if (ok) {
		return std::filesystem::path(buffer);
	} else {
		return std::filesystem::path();
	}
}

std::filesystem::path platform_save_file_dialog()
{
	WCHAR buffer[1024] = { '\0' };
	OPENFILENAMEW filename = { sizeof(OPENFILENAMEW) };
	filename.hwndOwner = ctx.hwnd;
	filename.lpstrFile = buffer;
	filename.nMaxFile = 1024;
	filename.lpstrFilter = L"CSV (*.csv)\0*.csv\0";
	filename.nFilterIndex = 1;
	filename.lpstrTitle = L"Save Results";
	filename.lpstrInitialDir = nullptr;
	filename.Flags = OFN_PATHMUSTEXIST;
	BOOL ok = GetSaveFileNameW(&filename);
	ImGui::GetIO().ClearInputKeys();
	if (ok) {
		return std::filesystem::path(buffer);
	} else {
		return std::filesystem::path();
	}
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

void platform_set_file_change_notifications(std::filesystem::path const &file)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	GetFileAttributesExW(file.wstring().c_str(), GetFileExInfoStandard, &data);
	ctx.last_file_write_time = data.ftLastWriteTime;
	ctx.monitoring_file = file;
}

bool platform_get_file_change_notification()
{
	if (!ctx.monitoring_file.empty()) {
		WIN32_FILE_ATTRIBUTE_DATA data;
		GetFileAttributesExW(ctx.monitoring_file.wstring().c_str(), GetFileExInfoStandard, &data);
		bool const changed = CompareFileTime(&ctx.last_file_write_time, &data.ftLastWriteTime) != 0;
		ctx.last_file_write_time = data.ftLastWriteTime;
		return changed;
	}
	return false;
}
