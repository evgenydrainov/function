#include "fonts/DroidSans.h"

#include <imgui_stdlib.h>
#include <imgui-sfml.h>
#include <imgui.h>

#include <SFML/Graphics.hpp>

static void LoadFont() {
	ImGuiIO& io = ImGui::GetIO();
	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
	builder.BuildRanges(&ranges);
	io.Fonts->AddFontFromMemoryCompressedTTF(DroidSans_compressed_data, DroidSans_compressed_size, 16, NULL, ranges.Data);
	ImGui::SFML::UpdateFontTexture();
}

int main() {
	sf::RenderWindow window(sf::VideoMode(640, 480), "function");
	window.setFramerateLimit(30);

	ImGui::SFML::Init(window, false);
	ImGui::StyleColorsLight();
	LoadFont();

	sf::Clock c;
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);
			if (event.type == sf::Event::Closed) {
				window.close();
			}
		}
		ImGui::SFML::Update(window, c.restart());

		if (ImGui::Begin("Hello")) {
			ImGui::Text("World");
		}
		ImGui::End();

		window.clear();
		ImGui::SFML::Render(window);
		window.display();
	}

	ImGui::SFML::Shutdown();
}
