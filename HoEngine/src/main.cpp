#include "common.h"
#include "input.h"
#define HOGL_IMPLEMENT
#include "ho_gl.h"
#include "util.h"
#include "hmath.h"
#include "application.h"
#include "WindowApi\Window.h"
#include "chat.h"
#include <stdlib.h>
#include <time.h>
#include "debug_table.h"

DebugTable debug_table;

Window_State win_state;

Keyboard_State keyboard_state = { 0 };
Mouse_State mouse_state = { 0 };
Chat* g_chat = 0;

LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_MOUSEMOVE: {
		int width = win_state.win_width;
		int height = win_state.win_height;
		int x = GET_X_LPARAM(lparam);
		int y = GET_Y_LPARAM(lparam);

		const float aspect = (float)width / height;
		float screenX = ((float)x * 2 / width - 1.0f);
		float screenY = -((float)y * 2 / height - 1.0f);

		mouse_state.x = x;
		mouse_state.y = y;
	}break;
	case WM_LBUTTONDOWN: {
		int width = win_state.win_width;
		int height = win_state.win_height;

		const float aspect = (float)width / height;
		float screenX = ((float)mouse_state.x * 2 / width - 1.0f);
		float screenY = -((float)mouse_state.x * 2 / height - 1.0f);

		for (unsigned int i = 0; i < linked::Window::openedWindows.size(); i++)
			linked::Window::openedWindows[i]->mouseCallback(0, 1, 0);
		linked::Button::mouseCallback(0, 1, 0);
	}break;
	case WM_LBUTTONUP: {
		for (unsigned int i = 0; i < linked::Window::openedWindows.size(); i++)
			linked::Window::openedWindows[i]->mouseCallback(0, 0, 0);
		linked::Button::mouseCallback(0, 0, 0);
	}break;
	case WM_KILLFOCUS: {
		ZeroMemory(keyboard_state.key, MAX_KEYS);
	}break;
	case WM_CREATE: break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SYSKEYDOWN:
		break;
	case WM_SYSKEYUP:
		break;
	case WM_CHAR:
		if (g_chat) {
			g_chat->handle_keystroke(wparam, lparam);
		}
		break;
	case WM_SIZE: {
		RECT r;
		GetClientRect(window, &r);
		win_state.win_width = r.right - r.left;
		win_state.win_height = r.bottom - r.top;
		glViewport(0, 0, win_state.win_width, win_state.win_height);
	} break;
	case WM_DROPFILES: {
		char buffer[512];
		HDROP hDrop = (HDROP)wparam;
		UINT ret = DragQueryFile(hDrop, 0, buffer, 512);
		POINT mouse_loc;
		DragQueryPoint(hDrop, &mouse_loc);
		DragFinish(hDrop);
		create_object(buffer);
	}break;
	default:
		return DefWindowProc(window, msg, wparam, lparam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
	WNDCLASSEX window_class;
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.style = 0;
	window_class.lpfnWndProc = WndProc;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = instance;
	window_class.hIcon = LoadIcon(instance, MAKEINTRESOURCE(NULL));
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	window_class.hbrBackground = 0;
	window_class.lpszMenuName = NULL;
	window_class.lpszClassName = "HoEngineClass";
	window_class.hIconSm = NULL;

	if (!RegisterClassEx(&window_class)) error_fatal("Error creating window class.\n", 0);

	// alloc console
#if DEBUG
	AllocConsole();
	FILE* pCout;
	freopen_s(&pCout, "CONOUT$", "w", stdout);
#endif

	u32 window_style_exflags = WS_EX_ACCEPTFILES | WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	u32 window_style_flags = WS_OVERLAPPEDWINDOW;
	// Note: Client area must be correct, so windows needs to get the WindowRect
	// area depending on the style of the window
	RECT window_rect = { 0 };
	window_rect.right = 1024;
	window_rect.bottom = 768;
	AdjustWindowRectEx(&window_rect, window_style_flags, FALSE, window_style_exflags);

	win_state.g_wpPrev.length = sizeof(WINDOWPLACEMENT);
	win_state.win_width = window_rect.right - window_rect.left;
	win_state.win_height = window_rect.bottom - window_rect.top;

	win_state.window_handle = CreateWindowExA(
		window_style_exflags,
		window_class.lpszClassName,
		"HoEngine",
		window_style_flags,
		CW_USEDEFAULT, CW_USEDEFAULT,
		win_state.win_width, win_state.win_height, NULL, NULL, instance, NULL
	);
	if (!win_state.window_handle) error_fatal("Error criating window context.\n", 0);

	ShowWindow(win_state.window_handle, cmd_show);
	UpdateWindow(win_state.window_handle);

	init_opengl(win_state.window_handle, &win_state.device_context, &win_state.rendering_context, 3, 1);

	init_application();

	wglSwapIntervalEXT_(1);		// Enable Vsync

	bool running = true;
	MSG msg;

	// Track mouse events
	TRACKMOUSEEVENT mouse_event = { 0 };
	mouse_event.cbSize = sizeof(mouse_event);
	mouse_event.dwFlags = TME_LEAVE;
	mouse_event.dwHoverTime = HOVER_DEFAULT;
	mouse_event.hwndTrack = win_state.window_handle;

	linked::Window::linkedWindowInit();

	using namespace linked;
	Chat chat;
	linked::Window* chat_window = chat.init_chat();
	g_chat = &chat;

	ParticleEmitter emitter(hm::vec3(10,10,10), 1.0f / 60.0f, 20, 2.0f);
	
	debug_table.create_entry("emitter_average_life", &emitter.average_life, TYPE_R32);
	debug_table.create_entry("emitter_particles_ps", &emitter.particles_ps, TYPE_R32);
	debug_table.create_entry("emitter_position_x", &emitter.emitter_pos.x, TYPE_R32);
	debug_table.create_entry("emitter_position_y", &emitter.emitter_pos.y, TYPE_R32);
	debug_table.create_entry("emitter_position_z", &emitter.emitter_pos.z, TYPE_R32);
	debug_table.create_entry("emitter_velocity_x", &emitter.velocity.x, TYPE_R32);
	debug_table.create_entry("emitter_velocity_y", &emitter.velocity.y, TYPE_R32);
	debug_table.create_entry("emitter_velocity_z", &emitter.velocity.z, TYPE_R32);
	debug_table.create_entry("emitter_velocity_variation_x", &emitter.velocity_variation.x, TYPE_R32);
	debug_table.create_entry("emitter_velocity_variation_y", &emitter.velocity_variation.y, TYPE_R32);
	debug_table.create_entry("emitter_velocity_variation_z", &emitter.velocity_variation.z, TYPE_R32);
	debug_table.create_entry("emitter_gravity_x", &emitter.gravity.x, TYPE_R32);
	debug_table.create_entry("emitter_gravity_y", &emitter.gravity.y, TYPE_R32);
	debug_table.create_entry("emitter_gravity_z", &emitter.gravity.z, TYPE_R32);

	srand(time(0));

	while (running) {
		TrackMouseEvent(&mouse_event);
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
			if (msg.message == WM_QUIT) {
				running = false;
				continue;
			}
			switch (msg.message) {
			case WM_KEYDOWN: {
				size_t key = msg.wParam;
				size_t mod = msg.lParam;
				keyboard_state.key[key] = true;
			} break;
			case WM_KEYUP: {
				size_t key = msg.wParam;
				keyboard_state.key[key] = false;
				keyboard_state.key_event[key] = true;
				if (keyboard_state.key_event[VK_UP] && g_chat->m_enabled) {
					keyboard_state.key_event[VK_UP] = false;
					g_chat->next_history();
				}
				if (keyboard_state.key_event[VK_DOWN] && g_chat->m_enabled) {
					keyboard_state.key_event[VK_DOWN] = false;
					g_chat->previous_history();
				}
			} break;
			case WM_MOUSEMOVE: {
				mouse_state.x = GET_X_LPARAM(msg.lParam);
				mouse_state.y = GET_Y_LPARAM(msg.lParam);

			} break;
			case WM_LBUTTONDOWN: {
				int x = GET_X_LPARAM(msg.lParam);
				int y = GET_Y_LPARAM(msg.lParam);
				mouse_state.is_captured = true;
				mouse_state.x_left = x;
				mouse_state.y_left = y;
				SetCapture(GetActiveWindow());
			} break;
			case WM_LBUTTONUP: {
				mouse_state.is_captured = false;
				ReleaseCapture();
			} break;
			case WM_CHAR: {
				size_t key = msg.wParam;
			} break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		win_state.move_camera = !chat_window->isAttached();
		win_state.do_input = !chat.m_enabled;

		emitter.emitt();

		update_and_render();
		emitter.update();
		emitter.render(render_object_default);

		chat.update();
		linked::Window::updateWindows();
		linked::Window::renderWindows();

		SwapBuffers(win_state.device_context);
	}
	linked::Window::linkedWindowDestroy();
#ifdef DEBUG
	FreeConsole();
#endif
	return 0;
}