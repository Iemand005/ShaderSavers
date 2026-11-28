#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ShaderSaver.hpp"
#include <window/SDLWindow.hpp>

#include <windows.h>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>

class ShaderSaverSDL : public ShaderSaverCore {
private:
	fe::SDLWindow* sdlWindow = nullptr;

protected:
	void PollInput() override {
		SDL_Event event;
		while (sdlWindow->PollSDLEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_KEY_DOWN:
					if (fullscreened) window->PrepareClose();
					break;
				case SDL_EVENT_QUIT:
					window->PrepareClose();
					break;
				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
					int w = 0, h = 0;
					SDL_GetWindowSize(sdlWindow->GetWindow(), &w, &h);
					Resize(w, h);
					break;
				}
				default:
					break;
			}
		}

		if (fullscreened) {
			double x = 0.0, y = 0.0;
			window->GetMousePosition(&x, &y);
			if (std::abs(x - startX) > 3.0 || std::abs(y - startY) > 3.0)
				window->PrepareClose();
		}
	}

	void Configure(ScreenSaverMode mode, void* parent) override {
		if (mode == ScreenSaverMode::Fullscreen) {
			fullscreened = true;
			sdlWindow->HideMouse();
			window->GetMousePosition(&startX, &startY);
		}

		if (mode == ScreenSaverMode::Preview) {
#ifdef _WIN32
			sdlWindow->attachToNativeParent(parent);
#endif
		} else {
			sdlWindow->Show();
		}
	}

	void InstallLiveResize(std::function<void()> draw) override {
#ifdef _WIN32
		sdlWindow->EnableLiveResizePump();
		sdlWindow->onLiveMoveResize = std::move(draw);
#else
		(void)draw;
#endif
	}

public:
	ShaderSaverSDL(bool fullscreen = false) : ShaderSaverSDL(500, 500, fullscreen) {}

	ShaderSaverSDL(int w, int h, bool fullscreen = false) : ShaderSaverCore(w, h, fullscreen) {
		auto created = std::make_unique<fe::SDLWindow>("ShaderSaver", w, h, false, fullscreen,
			fe::WindowOptions{}, false);
		sdlWindow = created.get();
		AttachWindow(std::move(created));
	}
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
	ScreenSaverMode mode = ScreenSaverMode::Window;
	HWND previewHwnd = nullptr;

	std::string cmd = lpCmdLine ? lpCmdLine : "";
	std::string cur;

	for (char c : cmd) {
		if (c == ' ') {
			if (!cur.empty()) {
				if (_stricmp(cur.c_str(), "/s") == 0)
					mode = ScreenSaverMode::Fullscreen;
				else if (_stricmp(cur.c_str(), "/c") == 0)
					mode = ScreenSaverMode::Config;
				else if (_stricmp(cur.c_str(), "/p") == 0)
					mode = ScreenSaverMode::Preview;
				else if (mode == ScreenSaverMode::Preview)
					previewHwnd = (HWND)std::stoull(cur);
				cur.clear();
			}
		} else {
			cur += c;
		}
	}

	if (!cur.empty()) {
		if (_stricmp(cur.c_str(), "/s") == 0)
			mode = ScreenSaverMode::Fullscreen;
		else if (_stricmp(cur.c_str(), "/c") == 0)
			mode = ScreenSaverMode::Config;
		else if (_stricmp(cur.c_str(), "/p") == 0)
			mode = ScreenSaverMode::Preview;
		else if (mode == ScreenSaverMode::Preview)
			previewHwnd = (HWND)std::stoull(cur);
	}

	ShaderSaverSDL game(mode == ScreenSaverMode::Fullscreen);
	game.Run(mode, previewHwnd);

	return 0;
}
