#pragma once
#include <SFML/Graphics.hpp>

enum class HAlign { Left, Center, Right };
enum class VAlign { Top, Middle, Bottom };

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

inline void AlignText(sf::Text& text, HAlign halign, VAlign valign, float offset = 0) {
	sf::FloatRect bounds = text.getLocalBounds();
	sf::Vector2f origin;
	switch (halign) {
		case HAlign::Left:
			origin.x = -offset;
			break;
		case HAlign::Center:
			origin.x = bounds.left + bounds.width / 2.0f;
			break;
		case HAlign::Right:
			origin.x = 2.0f * bounds.left + bounds.width + offset;
			break;
	}
	switch (valign) {
		case VAlign::Top:
			origin.y = -offset;
			break;
		case VAlign::Middle:
			origin.y = bounds.top + bounds.height / 2.0f;
			break;
		case VAlign::Bottom:
			origin.y = 2.0f * bounds.top + bounds.height + offset;
			break;
	}
	origin = std::floor(origin);
	text.setOrigin(origin);
}

/*
* https://github.com/eliasdaler/imgui-sfml/issues/119#issuecomment-622322382
*/
inline void CursorWorkaround(sf::RenderWindow& window) {
	static sf::Cursor cursor;
	static bool cursorLoaded = false;
	static bool imguiHasCursorPrev = true;
	if (!cursorLoaded) {
		cursor.loadFromSystem(sf::Cursor::Arrow);
		cursorLoaded = true;
	}
	bool imguiHasCursor = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
	if (imguiHasCursor != imguiHasCursorPrev) {
		if (imguiHasCursor) {
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
		} else {
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
			window.setMouseCursor(cursor);
		}
	}
	imguiHasCursorPrev = imguiHasCursor;
}
