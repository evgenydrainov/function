#pragma once
#include <SFML/Graphics.hpp>
#include <lua.hpp>

enum class HAlign { Left, Center, Right };
enum class VAlign { Top, Middle, Bottom };

class Program
{
public:
	Program();
	~Program();

	void run();

	void saveScript();
	void loadScript();
	void loadFonts();
	void tick();
	void renderGraph();

	sf::RenderWindow window;
	sf::RenderTexture graphSurf;
	sf::Vector2f view_pos;
	sf::Vector2f view_scale = sf::Vector2f(1, 1);
	std::string luaScript;
	std::string errorMsg;
	sf::Font font;
	lua_State* L;

	bool info_dragging = false;
	bool info_hovered = false;
};

void AlignText(sf::Text& text, HAlign halign, VAlign valign, float offset = 0);

/*
* https://github.com/eliasdaler/imgui-sfml/issues/119#issuecomment-622322382
*/
void CursorWorkaround(sf::RenderWindow& window);
