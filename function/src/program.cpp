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
	loadFont();
	loadScript();
	L = luaL_newstate();
	luaL_openlibs(L);
}

Program::~Program() {
	ImGui::SFML::Shutdown();
	lua_close(L);
	saveScript();
}

void Program::saveScript() {
	std::ofstream file("function.txt");
	file << luaScript;
}

void Program::loadScript() {
	std::ifstream file("function.txt");
	std::stringstream buffer;
	buffer << file.rdbuf();
	luaScript = buffer.str();
}

void Program::loadFont() {
	ImGuiIO& io = ImGui::GetIO();
	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
	builder.BuildRanges(&ranges);
	io.Fonts->AddFontFromMemoryCompressedTTF(DroidSans_compressed_data, DroidSans_compressed_size, 16, NULL, ranges.Data);
	ImGui::SFML::UpdateFontTexture();
}

void Program::tick() {
	info_dragging = false;
	info_hovered = false;

	if (ImGui::Begin(u8"График функции")) {
		sf::Vector2u region(std::max(1.0f, ImGui::GetContentRegionAvail().x),
							std::max(1.0f, ImGui::GetContentRegionAvail().y));
		if (graphSurf.getSize() != region) {
			graphSurf.create(region.x, region.y);
			renderGraph();
			fmt::print("Surface resized to ({}; {})\n", region.x, region.y);
		}
		ImGui::ImageButton(graphSurf, 0);
		if (ImGui::IsItemActive()) {
			sf::Vector2f off = ImGui::GetIO().MouseDelta;
			if (off != sf::Vector2f(0, 0)) {
				sf::View view = graphSurf.getView();
				view.move(-off);
				graphSurf.setView(view);
				renderGraph();
				fmt::print("Panned by ({}; {})\n", off.x, off.y);
			}
			info_dragging = true;
		}
		if (ImGui::IsItemHovered()) {
			if (ImGui::GetIO().MouseWheel != 0) {
				sf::View view = graphSurf.getView();
				if (ImGui::GetIO().MouseWheel > 0) {
					for (float i = 0; i <= ImGui::GetIO().MouseWheel; i += 1.0f) {
						view.zoom(0.9f);
					}
				} else {
					for (float i = 0; i >= ImGui::GetIO().MouseWheel; i -= 1.0f) {
						view.zoom(1.1f);
					}
				}
				graphSurf.setView(view);
				renderGraph();
				fmt::print("Zoomed by {}\n", ImGui::GetIO().MouseWheel);
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
	}
	ImGui::End();
}

void Program::renderGraph() {
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
			errorMsg += u8"Значение переменной \"f\" не является функцией.\n";
			return;
		}

		double screen_x = static_cast<double>(i);
		double x = screen_x - static_cast<double>(graphSurf.getSize().x / 2);

		lua_pushnumber(L, x);
		if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
			errorMsg += fmt::format("{}\n", lua_tostring(L, -1));
			continue;
		}

		if (!lua_isnumber(L, -1)) {
			errorMsg += fmt::format(u8"Значение функции \"f\" при аргументе {} не является числом.\n", x);
			continue;
		}

		double y = lua_tonumber(L, -1);
		double screen_y = static_cast<double>(graphSurf.getSize().y / 2) - y;
		vertices.append(sf::Vertex(sf::Vector2f(screen_x, screen_y)));
	}

	graphSurf.clear();
	graphSurf.draw(vertices);
	graphSurf.display();
	fmt::print("Rerendered...\n");
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
