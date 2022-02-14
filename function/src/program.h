#pragma once
#include <SFML/Graphics.hpp>

#include <imgui_stdlib.h>
#include <imgui-sfml.h>
#include <imgui.h>

#include <lua.hpp>

enum class HAlign { Left, Center, Right };
enum class VAlign { Top, Middle, Bottom };

class Program
{
public:
	Program();
	~Program();

	void tick();
	void renderGraph();
	void run();

	sf::RenderWindow window;
	sf::RenderTexture graphSurf;
	sf::Vector2f view_offset = sf::Vector2f(0, 0);
	sf::Vector2f view_scale = sf::Vector2f(0.02, 0.02);
	std::string luaScript;
	std::string errorMsg;
	sf::Font font;
	lua_State* L;

	bool info_dragging = false;
	bool info_hovered = false;

	ImVec4 graphCol = ImVec4(0, 0, 0, 1); // normalized
	ImVec4 bgCol = ImVec4(1, 1, 1, 1);

	int fps = 30;
};

void AlignText(sf::Text& text, HAlign halign, VAlign valign, float offset = 0);

/*
* https://github.com/eliasdaler/imgui-sfml/issues/119#issuecomment-622322382
*/
void CursorWorkaround(sf::RenderWindow& window);

namespace std
{
	template <typename T>
	sf::Vector2<T> floor(sf::Vector2<T> v) {
		return sf::Vector2<T>(floor(v.x), floor(v.y));
	}

	template <typename T>
	sf::Vector2<T> round(sf::Vector2<T> v) {
		return sf::Vector2<T>(round(v.x), round(v.y));
	}

	template <typename T>
	sf::Vector2<T> ceil(sf::Vector2<T> v) {
		return sf::Vector2<T>(ceil(v.x), ceil(v.y));
	}

	template <typename T>
	sf::Vector2<T> min(sf::Vector2<T> a, sf::Vector2<T> b) {
		return sf::Vector2<T>(min(a.x, b.x), min(a.y, b.y));
	}

	template <typename T>
	sf::Vector2<T> max(sf::Vector2<T> a, sf::Vector2<T> b) {
		return sf::Vector2<T>(max(a.x, b.x), max(a.y, b.y));
	}

	template <typename T>
	T floor_to(T x, T n) {
		return floor(x / n) * n;
	}

	template <typename T>
	T round_to(T x, T n) {
		return round(x / n) * n;
	}

	template <typename T>
	T ceil_to(T x, T n) {
		return ceil(x / n) * n;
	}
}

template <typename T>
sf::Vector2<T> operator *(sf::Vector2<T> rhs, sf::Vector2<T> lhs) {
	return sf::Vector2<T>(rhs.x * lhs.x, rhs.y * lhs.y);
}

template <typename T>
sf::Vector2<T> operator /(sf::Vector2<T> rhs, sf::Vector2<T> lhs) {
	return sf::Vector2<T>(rhs.x / lhs.x, rhs.y / lhs.y);
}
