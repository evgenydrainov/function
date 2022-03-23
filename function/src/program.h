#pragma once
#include <SFML/Graphics.hpp>

#include <imgui_stdlib.h>
#include <imgui-sfml.h>
#include <imgui.h>

#include <lua.hpp>

class Program
{
public:
	Program();
	~Program();

	void tick();
	void renderGraph();
	void run();
	void recreateFont();

	sf::RenderWindow window;
	sf::RenderTexture graphSurf;
	sf::Vector2f view_offset = sf::Vector2f(0, 0);
	sf::Vector2f view_scale = sf::Vector2f(0.02, 0.02);
	std::string luaScript;
	std::string errorMsg;
	sf::Font font;
	lua_State* L = nullptr;
	bool needRecreateFont = true;
	float fontSize = 16;
	int labelFontSize = 12;

	bool info_dragging = false;
	bool info_hovered = false;
	bool show_info = false;

	ImVec4 graphCol = ImVec4(0, 0, 0, 1); // normalized
	ImVec4 bgCol = ImVec4(1, 1, 1, 1);

	int fps = 30;
};
