#pragma once
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <window/IWindow.hpp>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <window/GLFW3Window.hpp>
#include <Graphics/IRenderDevice.hpp>
#include <Graphics/OpenGLRenderDevice.hpp>
#include <ShaderProgram.hpp>
#include <ScreenSaverMode.hpp>
#include <glad/glad.h>

#include <cmath>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <utility>

class ShaderSaverCore {
protected:
	static constexpr const char* kVertexShader = R"(
		#version 330 core
		void main() {
			vec2 v[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));
			gl_Position = vec4(v[gl_VertexID], 0, 1);
		})";

	std::unique_ptr<fe::IWindow> ownedWindow;
	std::unique_ptr<fe::IRenderDevice> device;
	std::unique_ptr<fe::ShaderProgram> shader;
	fe::IWindow* window = nullptr;
	GLuint vao = 0;
	int width = 0, height = 0;
	double startX = 0.0, startY = 0.0;
	bool fullscreened = false;
	std::string fragmentPath = "FragmentShader.glsl";
	std::filesystem::file_time_type lastWrite;

	void AttachWindow(std::unique_ptr<fe::IWindow> newWindow) {
		device->init(newWindow.get());
		window = newWindow.get();
		ownedWindow = std::move(newWindow);
	}

	virtual void PollInput() = 0;
	virtual void Configure(ScreenSaverMode mode, void* parent) { (void)mode; (void)parent; }
	virtual void InstallLiveResize(std::function<void()> draw) { (void)draw; }
	virtual void DrawFrame() { glDrawArrays(GL_TRIANGLES, 0, 3); }
	virtual void Cleanup() {}

	static std::filesystem::file_time_type GetFileTime(const char* path) {
		std::error_code ec;
		return std::filesystem::last_write_time(path, ec);
	}

	void Reload() {
		try {
			shader = std::make_unique<fe::ShaderProgram>(
				fe::Shader::Vertex(kVertexShader), fe::Shader::Fragment(fragmentPath));
			if (!shader->IsLinked()) return;
			shader->Use();
			device->SetInt("prevFrame", 0);
		} catch (...) {
		}
	}

	void HotReload() {
		auto now = GetFileTime(fragmentPath.c_str());
		if (now != lastWrite) {
			lastWrite = now;
			Reload();
		}
	}

	virtual void Resize(int w, int h) {
		if (w <= 0 || h <= 0) return;
		width = w;
		height = h;
		device->Resize(w, h);
	}

	void RenderFrame() {
		if (!shader || !shader->IsLinked()) return;

		shader->Use();
		device->SetFloat("time", static_cast<float>(window->GetTime()));
		device->SetInt("prevFrame", 0);
		device->SetClearColor(0.01f, 0.0f, 0.03f, 1.0f);
		device->clear();
		device->DisableDepthTest();
		device->Resize(width, height);
		glBindVertexArray(vao);

		DrawFrame();
		window->SwapBuffers();
	}

public:
	ShaderSaverCore(int w, int h, bool fullscreen)
		: width(w), height(h), fullscreened(fullscreen), device(std::make_unique<fe::OpenGLRenderDevice>()) {}

	virtual ~ShaderSaverCore() = default;

	void Run(ScreenSaverMode mode = ScreenSaverMode::Window, void* parent = nullptr) {
		Configure(mode, parent);

		lastWrite = GetFileTime(fragmentPath.c_str());
		Reload();

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		auto size = window->GetFramebufferSize();
		Resize(size.width, size.height);

		InstallLiveResize([this]() { RenderFrame(); });

		while (!window->shouldClose()) {
			PollInput();
			if (mode != ScreenSaverMode::Fullscreen) HotReload();
			RenderFrame();
		}

		glDeleteVertexArrays(1, &vao);
		Cleanup();
		window->Destroy();
	}
};

class ShaderSaver : public ShaderSaverCore {
private:
	fe::GLFW3Window* glfwWindow = nullptr;

protected:
	void PollInput() override {
		glfwWindow->PollGLFWEvents();

		if (fullscreened) {
			double x = 0.0, y = 0.0;
			window->GetMousePosition(&x, &y);
			if (std::abs(x - startX) > 3.0 || std::abs(y - startY) > 3.0)
				window->PrepareClose();
		}
	}

	void Configure(ScreenSaverMode mode, void* parent) override {
		if (fullscreened)
			glfwWindow->SetAnyKeyCallback([this]() { window->PrepareClose(); });
		else
			glfwWindow->SetFramebufferResizeCallback([this](int w, int h) { Resize(w, h); });

		if (mode == ScreenSaverMode::Fullscreen) {
			fullscreened = true;
			glfwWindow->HideMouse();
			window->GetMousePosition(&startX, &startY);
		}

		if (mode == ScreenSaverMode::Preview) {
#ifdef _WIN32
			glfwWindow->AttachToNativeParent(parent);
#endif
		} else {
			glfwWindow->Show();
		}
	}

	void InstallLiveResize(std::function<void()> draw) override {
#ifdef _WIN32
		glfwWindow->EnableLiveResizePump();
		glfwWindow->onLiveMoveResize = std::move(draw);
#else
		(void)draw;
#endif
	}

public:
	ShaderSaver(bool fullscreen = false) : ShaderSaver(500, 500, fullscreen) {}

	ShaderSaver(int w, int h, bool fullscreen = false) : ShaderSaverCore(w, h, fullscreen) {
		auto created = std::make_unique<fe::GLFW3Window>("ShaderSaver", w, h, false, fullscreen);
		glfwWindow = created.get();
		AttachWindow(std::move(created));
	}
};
