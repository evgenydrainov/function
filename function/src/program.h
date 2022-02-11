#pragma once
#include <SFML/Graphics.hpp>
#include <lua.hpp>

class Program
{
public:
	Program();
	~Program();

	void run();

	void saveScript();
	void loadScript();
	void loadFont();
	void tick();
	void renderGraph();

	sf::RenderWindow window;
	sf::RenderTexture graphSurf;
	std::string luaScript;
	std::string errorMsg;
	lua_State* L;

	bool info_dragging = false;
	bool info_hovered = false;
};

/*
* https://github.com/eliasdaler/imgui-sfml/issues/119#issuecomment-622322382
*/
void CursorWorkaround(sf::RenderWindow& window);
