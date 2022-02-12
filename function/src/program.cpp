#include "program.h"

#include "fonts/DroidSans.h"

#include <imgui_stdlib.h>
#include <imgui-sfml.h>
#include <imgui.h>

#include "fmt/format.h"

#include <fstream>
#include <sstream>

Program::Program() {
	window.create(sf::VideoMode(640, 480), L"function");
	window.setFramerateLimit(30);
	ImGui::SFML::Init(window, false);
	ImGui::StyleColorsLight();
	loadFonts();
	loadScript();
	L = luaL_newstate();
	luaL_openlibs(L);
	fmt::print("Initialized.\n");
}

Program::~Program() {
	ImGui::SFML::Shutdown();
	lua_close(L);
	saveScript();
	fmt::print("Shut down.\n");
}

void Program::saveScript() {
	std::ofstream file("function.txt");
	file << luaScript;
	fmt::print("Script saved.\n");
}

void Program::loadScript() {
	std::ifstream file("function.txt");
	std::stringstream buffer;
	buffer << file.rdbuf();
	luaScript = buffer.str();
	fmt::print("Script loaded.\n");
}

void Program::loadFonts() {
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;
	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
	builder.BuildRanges(&ranges);
	io.Fonts->AddFontFromMemoryTTF((void*)DroidSans_data, DroidSans_size, 16, &config, ranges.Data);
	font.loadFromMemory(DroidSans_data, DroidSans_size);
	ImGui::SFML::UpdateFontTexture();
	fmt::print("Fonts loaded.\n");
}

void Program::tick() {
	info_dragging = false;
	info_hovered = false;

	if (ImGui::Begin(u8"График функции")) {
		sf::Vector2u region(std::max(1.0f, ImGui::GetContentRegionAvail().x),
							std::max(1.0f, ImGui::GetContentRegionAvail().y));
		if (graphSurf.getSize() != region) {
			static bool centered = false;
			if (!centered) {
				//view_pos = sf::Vector2f(region / 2u);
				centered = true;
			}
			graphSurf.create(region.x, region.y);
			fmt::print("Surface resized to ({}; {})\n", region.x, region.y);
			renderGraph();
		}
		ImGui::ImageButton(graphSurf, 0);
		if (ImGui::IsItemActive()) {
			sf::Vector2f off = ImGui::GetIO().MouseDelta;
			if (off != sf::Vector2f(0, 0)) {
				view_pos += off;
				//sf::View view = graphSurf.getView();
				//view.move(-off);
				//graphSurf.setView(view);
				fmt::print("Panned by ({}; {})\n", off.x, off.y);
				renderGraph();
			}
			info_dragging = true;
		}
		if (ImGui::IsItemHovered()) {
			if (ImGui::GetIO().MouseWheel != 0) {
				if (ImGui::GetIO().MouseWheel > 0) {
					for (float i = 0; i < ImGui::GetIO().MouseWheel; i += 1.0f) {
						view_scale.x *= 0.9f;
						view_scale.y *= 0.9f;
						//sf::View view = graphSurf.getView();
						//view.zoom(0.9f);
						//graphSurf.setView(view);
					}
				} else {
					for (float i = 0; i > ImGui::GetIO().MouseWheel; i -= 1.0f) {
						view_scale.x *= 1.1f;
						view_scale.y *= 1.1f;
						//sf::View view = graphSurf.getView();
						//view.zoom(1.1f);
						//graphSurf.setView(view);
					}
				}
				fmt::print("Zoomed by {}\n", ImGui::GetIO().MouseWheel);
				renderGraph();
			}
			info_hovered = true;
		}
	}
	ImGui::End();

	if (ImGui::Begin(u8"Программа")) {
		ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
		if (ImGui::InputTextMultiline(u8"Код Lua", &luaScript, ImVec2(0, contentRegionAvail.y), ImGuiInputTextFlags_AllowTabInput)) {
			renderGraph();
		}
	}
	ImGui::End();

	if (ImGui::Begin(u8"Сообщение об ошибке")) {
		ImGui::Text("%s", errorMsg.c_str());
	}
	ImGui::End();

	if (ImGui::Begin(u8"Информация")) {
		ImGui::Text("dragging: %d", info_dragging);
		ImGui::Text("hovered: %d", info_hovered);
		ImGui::Text("lua top: %d", lua_gettop(L));
		ImGui::Text("view pos: (%f; %f)", view_pos.x, view_pos.y);
		ImGui::Text("view scale: (%f; %f)", view_scale.x, view_scale.y);
	}
	ImGui::End();

	//ImGui::ShowDemoWindow();
}

void Program::renderGraph() {
	fmt::print("Rerendering...\n");

	errorMsg.clear();
	if (luaL_dostring(L, luaScript.c_str()) != LUA_OK) {
		errorMsg += fmt::format("{}\n", lua_tostring(L, -1));
		return;
	}

	lua_settop(L, 0);

	sf::VertexArray vertices(sf::Points);
	for (unsigned i = 0; i < graphSurf.getSize().x; i++) {
		lua_getglobal(L, "f");
		if (!lua_isfunction(L, -1)) {
			errorMsg += fmt::format(u8"Значение переменной \"f\" не является функцией ({}).\n", luaL_typename(L, -1));
			break;
		}

		double screen_x = static_cast<double>(i);
		double fx = screen_x - view_pos.x;
		fx *= view_scale.x;

		lua_pushnumber(L, fx);
		if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
			errorMsg += fmt::format("{}\n", lua_tostring(L, -1));
			continue;
		}

		if (!lua_isnumber(L, -1)) {
			errorMsg += fmt::format(u8"Значение функции \"f\" при аргументе {} не является числом ({}).\n", fx, luaL_typename(L, -1));
			continue;
		}

		double fy = lua_tonumber(L, -1);
		fy /= view_scale.y;
		double screen_y = view_pos.y - fy;
		vertices.append(sf::Vertex(sf::Vector2f(screen_x, screen_y), sf::Color::Black));
	}

	graphSurf.clear(sf::Color::White);

	sf::VertexArray axis(sf::Lines);
	axis.append(sf::Vertex(sf::Vector2f(0, view_pos.y), sf::Color::Black));
	axis.append(sf::Vertex(sf::Vector2f(graphSurf.getSize().x, view_pos.y), sf::Color::Black));
	axis.append(sf::Vertex(sf::Vector2f(view_pos.x, 0), sf::Color::Black));
	axis.append(sf::Vertex(sf::Vector2f(view_pos.x, graphSurf.getSize().y), sf::Color::Black));
	graphSurf.draw(axis);

	graphSurf.draw(vertices);

	sf::Text text;
	text.setFont(font);
	text.setCharacterSize(12);
	text.setFillColor(sf::Color::Black);
	text.setString("0");
	text.setPosition(view_pos.x, view_pos.y);
	AlignText(text, HAlign::Right, VAlign::Top, 2);
	graphSurf.draw(text);

	graphSurf.display();
	fmt::print("Rerendered.\n");
}

void Program::run() {
	sf::Clock clock;
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);
			if (event.type == sf::Event::Closed) {
				window.close();
			}
		}
		ImGui::SFML::Update(window, clock.restart());
		tick();
		CursorWorkaround(window);
		window.clear();
		ImGui::SFML::Render(window);
		window.display();
	}
}

void AlignText(sf::Text& text, HAlign halign, VAlign valign, float offset) {
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
	origin.x = std::floor(origin.x);
	origin.y = std::floor(origin.y);
	text.setOrigin(origin);
}

void CursorWorkaround(sf::RenderWindow& window)
{
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
