#include "program.h"

#include "fonts/DroidSans.h"

#include <fmt/format.h>

#include <inipp/inipp.h>

#include <fstream>
#include <sstream>

#include "misc.h"

Program::Program() {
	window.create(sf::VideoMode(640, 480), L"function");
	ImGui::SFML::Init(window, false);
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsLight();
	ImGui::SetColorEditOptions(ImGuiColorEditFlags_PickerHueWheel);

	font.loadFromMemory(DroidSans_data, DroidSans_size);

	{
		std::ifstream file("function.txt");
		if (file.is_open()) {
			std::stringstream buf;
			buf << file.rdbuf();
			luaScript = buf.str();
		} else {
			luaScript = "function f(x)\n"
				"\treturn x\n"
				"end";
		}
	}

	{
		inipp::Ini<char> ini;
		std::ifstream file("function.ini");
		ini.parse(file);
		unsigned uGraphCol, uBgCol;
		if (inipp::get_value(ini.sections["Colors"], "GraphColor", uGraphCol)) {
			graphCol = ImGui::ColorConvertU32ToFloat4(uGraphCol);
		}
		if (inipp::get_value(ini.sections["Colors"], "BackgroundColor", uBgCol)) {
			bgCol = ImGui::ColorConvertU32ToFloat4(uBgCol);
		}

		if (ini.sections["Rendering"]["FPS"] == "VSync") {
			fps = 0;
		} else if (ini.sections["Rendering"]["FPS"] == "Unlocked") {
			fps = -1;
		} else {
			inipp::get_value(ini.sections["Rendering"], "FPS", fps);
		}
		inipp::get_value(ini.sections["Rendering"], "Precision", precision);

		if (fps == 0) {
			window.setVerticalSyncEnabled(true);
			window.setFramerateLimit(0);
		} else if (fps > 0) {
			window.setVerticalSyncEnabled(false);
			window.setFramerateLimit(fps);
		} else {
			window.setVerticalSyncEnabled(false);
			window.setFramerateLimit(0);
		}

		inipp::get_value(ini.sections["Other"], "ShowInfo", show_info);
		inipp::get_value(ini.sections["Other"], "FontSize", fontSize);
		inipp::get_value(ini.sections["Other"], "LabelFontSize", labelFontSize);
	}

	L = luaL_newstate();
	luaL_openlibs(L);
	//fmt::print("Initialized.\n");
}

Program::~Program() {
	ImGui::SFML::Shutdown();
	if (L) {
		lua_close(L);
		L = nullptr;
	}

	{
		std::ofstream file("function.txt");
		file << luaScript;
	}

	{
		inipp::Ini<char> ini;
		ini.sections["Colors"]["GraphColor"] = fmt::format("{}", ImGui::ColorConvertFloat4ToU32(graphCol));
		ini.sections["Colors"]["BackgroundColor"] = fmt::format("{}", ImGui::ColorConvertFloat4ToU32(bgCol));

		if (fps == 0) {
			ini.sections["Rendering"]["FPS"] = "VSync";
		} else if (fps < 0) {
			ini.sections["Rendering"]["FPS"] = "Unlocked";
		} else {
			ini.sections["Rendering"]["FPS"] = fmt::format("{}", fps);
		}
		ini.sections["Rendering"]["Precision"] = fmt::format("{}", precision);

		ini.sections["Other"]["ShowInfo"] = show_info ? "true" : "false";
		ini.sections["Other"]["FontSize"] = fmt::format("{}", fontSize);
		ini.sections["Other"]["LabelFontSize"] = fmt::format("{}", labelFontSize);

		std::ofstream file("function.ini");
		file << "; Colors are ABGR\n"
			"; FPS: \"VSync\", \"Unlocked\" or any value\n";
		ini.generate(file);
	}

	//fmt::print("Shut down.\n");
}

void Program::tick() {
	info_dragging = false;
	info_hovered = false;

	ImGui::DockSpaceOverViewport(NULL, ImGuiDockNodeFlags_PassthruCentralNode);

	if (ImGui::Begin(u8"График функции")) {
		sf::Vector2u region(std::max(sf::Vector2f(1, 1), sf::Vector2f(ImGui::GetContentRegionAvail())));
		if (graphSurf.getSize() != region) {
			graphSurf.create(region.x, region.y);
			//fmt::print("Surface resized to ({}; {})\n", region.x, region.y);
			renderGraph();
		}
		ImGui::ImageButton(graphSurf, 0);
		if (ImGui::IsItemActive()) {
			sf::Vector2f off = ImGui::GetIO().MouseDelta;
			if (off != sf::Vector2f(0, 0)) {
				view_offset -= off * view_scale;
				//fmt::print("Panned by ({}; {})\n", off.x, off.y);
				renderGraph();
			}
			info_dragging = true;
		}
		if (ImGui::IsItemHovered()) {
			if (ImGui::GetIO().MouseWheel != 0) {
				if (ImGui::GetIO().MouseWheel > 0) {
					for (float i = 0; i < ImGui::GetIO().MouseWheel; i += 1.0f) {
						view_scale *= 0.9f;
					}
				} else {
					for (float i = 0; i > ImGui::GetIO().MouseWheel; i -= 1.0f) {
						view_scale *= 1.1f;
					}
				}
				//fmt::print("Zoomed by {}\n", ImGui::GetIO().MouseWheel);
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

	if (show_info) {
		if (ImGui::Begin("info")) {
			ImGui::Text("dragging: %d", info_dragging);
			ImGui::Text("hovered: %d", info_hovered);
			ImGui::Text("lua top: %d", lua_gettop(L));
			bool rerender = false;
			rerender |= ImGui::InputFloat2("view offset", &view_offset.x);
			rerender |= ImGui::InputFloat2("view scale", &view_scale.x);
			if (rerender) {
				renderGraph();
			}
		}
		ImGui::End();
	}

	if (ImGui::Begin(u8"Настройки")) {
		bool rerender = false;
		rerender |= ImGui::ColorEdit4(u8"Цвет графика", &graphCol.x, ImGuiColorEditFlags_NoInputs);
		rerender |= ImGui::ColorEdit4(u8"Цвет заднего фона", &bgCol.x, ImGuiColorEditFlags_NoInputs);
		if (rerender) {
			renderGraph();
		}

		ImGui::PushItemWidth(100);
		if (ImGui::DragFloat(u8"Размер шрифта текста", &fontSize, 0.1f, 1.0f, 100.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
			needRecreateFont = true;
		}
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(100);
		if (ImGui::DragInt(u8"Размер шрифта координат", &labelFontSize, 0.1f, 1, 100, NULL, ImGuiSliderFlags_AlwaysClamp)) {
			renderGraph();
		}
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(100);
		if (ImGui::DragInt(u8"Точность", &precision, 0.05f, 1, 50, NULL, ImGuiSliderFlags_AlwaysClamp)) {
			renderGraph();
		}
		ImGui::PopItemWidth();
	}
	ImGui::End();

	//ImGui::ShowDemoWindow();
}

void Program::renderGraph() {
	//fmt::print("Rerendering...\n");

	errorMsg.clear();
	if (luaL_loadstring(L, luaScript.c_str()) != LUA_OK) {
		errorMsg += fmt::format(u8"Синтаксическая ошибка:\n{}\n", lua_tostring(L, -1));
		return;
	}

	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		errorMsg += fmt::format(u8"Ошибка выполнения:\n{}\n", lua_tostring(L, -1));
		return;
	}

	// look into how to properly manage lua stack
	lua_settop(L, 0);

	sf::View view;
	view.setSize(sf::Vector2f(graphSurf.getSize()) * view_scale);
	view.setCenter(view_offset);
	graphSurf.setView(view);

	sf::Vector2f view_pos = view.getCenter() - view.getSize() / 2.0f;

	sf::VertexArray vertices(sf::Points);
	for (unsigned i = 0; i < graphSurf.getSize().x * precision; i++) {
		lua_getglobal(L, "f");
		if (!lua_isfunction(L, -1)) {
			errorMsg += fmt::format(u8"Значение переменной \"f\" не является функцией ({}).\n", luaL_typename(L, -1));
			break;
		}

		double fx = view_pos.x + double(i) / double(precision) / double(graphSurf.getSize().x) * view.getSize().x;

		lua_pushnumber(L, fx);
		if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
			errorMsg += fmt::format(u8"Ошибка выполнения при аргументе {}:\n{}\n", fx, lua_tostring(L, -1));
			continue;
		}

		if (!lua_isnumber(L, -1)) {
			errorMsg += fmt::format(u8"Значение функции \"f\" при аргументе {} не является числом ({}).\n", fx, luaL_typename(L, -1));
			continue;
		}

		double fy = lua_tonumber(L, -1);
		vertices.append(sf::Vertex(sf::Vector2f(fx, -fy), graphCol));
	}

	graphSurf.clear(bgCol);

	float xrel = 150.0f * view_scale.x;
	float xstep = std::max(1000.0f, std::floor_to(xrel, 1000.0f));
	if (xrel < 1) {
		xstep = 1;
	} else if (xrel < 2) {
		xstep = 2;
	} else if (xrel < 5) {
		xstep = 5;
	} else if (xrel < 10) {
		xstep = 10;
	} else if (xrel < 20) {
		xstep = 20;
	} else if (xrel < 50) {
		xstep = 50;
	} else if (xrel < 100) {
		xstep = 100;
	} else if (xrel < 200) {
		xstep = 200;
	} else if (xrel < 500) {
		xstep = 500;
	}

	float yrel = 150.0f * view_scale.y;
	float ystep = std::max(1000.0f, std::floor_to(yrel, 1000.0f));
	if (yrel < 1) {
		ystep = 1;
	} else if (yrel < 2) {
		ystep = 2;
	} else if (yrel < 5) {
		ystep = 5;
	} else if (yrel < 10) {
		ystep = 10;
	} else if (yrel < 20) {
		ystep = 20;
	} else if (yrel < 50) {
		ystep = 50;
	} else if (yrel < 100) {
		ystep = 100;
	} else if (yrel < 200) {
		ystep = 200;
	} else if (yrel < 500) {
		ystep = 500;
	}

	sf::VertexArray axes(sf::Lines);
	for (float axis_x = std::floor_to(view_pos.x, xstep); axis_x <= view_pos.x + view.getSize().x; axis_x += xstep) {
		axes.append(sf::Vertex(sf::Vector2f(axis_x, view_pos.y), sf::Color(128, 128, 255, 128)));
		axes.append(sf::Vertex(sf::Vector2f(axis_x, view_pos.y + view.getSize().y), sf::Color(128, 128, 255, 128)));
	}

	for (float axis_y = std::floor_to(view_pos.y, ystep); axis_y <= view_pos.y + view.getSize().y; axis_y += ystep) {
		axes.append(sf::Vertex(sf::Vector2f(view_pos.x, axis_y), sf::Color(128, 128, 255, 128)));
		axes.append(sf::Vertex(sf::Vector2f(view_pos.x + view.getSize().x, axis_y), sf::Color(128, 128, 255, 128)));
	}

	graphSurf.draw(axes);

	graphSurf.draw(vertices);

	for (float label_x = std::floor_to(view_pos.x, xstep); label_x <= view_pos.x + view.getSize().x + 50.0f; label_x += xstep) {
		sf::Text text;
		text.setFont(font);
		text.setCharacterSize(labelFontSize);
		text.setFillColor(sf::Color::Black);
		text.setScale(view_scale);
		text.setString(fmt::format("{}", label_x));
		text.setPosition(label_x, 0);
		AlignText(text, HAlign::Right, VAlign::Top, 2);
		graphSurf.draw(text);
	}

	for (float label_y = std::floor_to(view_pos.y, ystep); label_y <= view_pos.y + view.getSize().y + 50.0f; label_y += ystep) {
		if (label_y == 0) {
			continue;
		}
		sf::Text text;
		text.setFont(font);
		text.setCharacterSize(labelFontSize);
		text.setFillColor(sf::Color::Black);
		text.setScale(view_scale);
		text.setString(fmt::format("{}", -label_y));
		text.setPosition(0, label_y);
		AlignText(text, HAlign::Right, VAlign::Top, 2);
		graphSurf.draw(text);
	}

	graphSurf.display();
	//fmt::print("Rerendered.\n");
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
		if (needRecreateFont) {
			recreateFont();
			needRecreateFont = false;
		}
		ImGui::SFML::Update(window, clock.restart());
		tick();
		CursorWorkaround(window);
		window.clear();
		ImGui::SFML::Render(window);
		if (!window.hasFocus()) {
			sf::RectangleShape r;
			r.setFillColor(sf::Color(0, 0, 0, 128));
			r.setSize(sf::Vector2f(window.getSize()));
			window.draw(r);
		}
		window.display();
	}
}

void Program::recreateFont() {
	ImGuiIO& io = ImGui::GetIO();

	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;

	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
	builder.BuildRanges(&ranges);

	io.Fonts->Clear();
	io.Fonts->AddFontFromMemoryTTF((void*)DroidSans_data, DroidSans_size, fontSize, &config, ranges.Data);

	ImGui::SFML::UpdateFontTexture();
}
