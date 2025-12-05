#pragma once

#include <functional>
#include <string>
#include <thread>
#include <Windows.h>
#include "../ImGui/imgui.h"

class Logger;

class GameWindow {
private:
	HWND hwnd;
	float x, y, width, height;
public:
	inline float getWidth() {
		return width;
	}
	inline float getHeight() {
		return height;
	}
	inline HWND getHwnd() {
		return hwnd;
	}
	static GameWindow* createInstance(LPCWSTR name);
	void drawLine(ImVec2 const& start, ImVec2 const& end, ImColor const& color);
	void drawCircle(ImVec2 const& center, float radius, ImColor const& color);
	ImVec2 drawText(float screen_x, float screen_y, const char* text, ImColor const& color);
	void drawESPLine(float screen_x, float screen_y, ImColor const& color);
	void drawESPLine(ImVec2 const& screen_pos, ImColor const& color);
	void drawESPLineWithBox(float screen_x, float screen_y, float line_height, float distance, ImColor const& color);
	void drawESPBox(float screen_x, float screen_y, ImVec2 const& size, float distance, ImColor const& color);
	void drawCenterCircle(float radius, ImColor const& color);
};

class Render {
private:
	std::thread render_thread;

	std::wstring game_window_name;
	Logger* logger;
	std::function<void(GameWindow*)> logic;
public:
	static Render* getInstance();
	void setup(std::wstring const& game_window_name,Logger* logger, std::function<void(GameWindow*)> logic);
	void start();
};