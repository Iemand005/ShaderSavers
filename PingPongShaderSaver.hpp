#pragma once
#include "ShaderSaver.hpp"

class PingPongShaderSaver : public ShaderSaver {
private:
	uint64_t textures[2] = {0, 0};
	GLuint framebuffers[2] = {0, 0};
	int readIndex = 0, writeIndex = 1;
	int targetWidth = 0, targetHeight = 0;

	void RecreateTargets(int w, int h) {
		if (w <= 0 || h <= 0 || (w == targetWidth && h == targetHeight)) return;
		ReleaseTargets();

		targetWidth = w;
		targetHeight = h;

		for (int i = 0; i < 2; ++i) {
			textures[i] = device->CreateColorAttachment(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
			glGenFramebuffers(1, &framebuffers[i]);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				static_cast<GLuint>(textures[i]), 0);
			device->clear();
		}
		device->BindFramebuffer(0);
	}

	void ReleaseTargets() {
		for (int i = 0; i < 2; ++i) {
			if (framebuffers[i]) {
				device->DestroyFramebuffer(framebuffers[i]);
				framebuffers[i] = 0;
			}
			if (textures[i]) {
				device->DestroyColorAttachment(textures[i]);
				textures[i] = 0;
			}
		}
		targetWidth = targetHeight = 0;
	}

protected:
	void Resize(int w, int h) override {
		ShaderSaverCore::Resize(w, h);
		RecreateTargets(w, h);
	}

	void DrawFrame() override {
		device->SetInt("prevFrame", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textures[readIndex]));

		device->BindFramebuffer(static_cast<int>(framebuffers[writeIndex]));
		device->Resize(width, height);
		device->clear();
		glDrawArrays(GL_TRIANGLES, 0, 3);

		std::swap(readIndex, writeIndex);

		device->BindFramebuffer(0);
		device->Resize(width, height);
		glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textures[readIndex]));
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	void Cleanup() override {
		ReleaseTargets();
	}

public:
	PingPongShaderSaver(bool fullscreen = false) : PingPongShaderSaver(500, 500, fullscreen) {}

	PingPongShaderSaver(int w, int h, bool fullscreen = false) : ShaderSaver(w, h, fullscreen) {
		fragmentPath = "FragmentShaderTest.glsl";
		RecreateTargets(w, h);
	}
};
