#include "Algoritmos.hpp"
#include "Estructuras.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

int main() {

  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800, 600)),
                          "Logistica VRP - GRUPO 2");
  window.setFramerateLimit(60);

  if (!ImGui::SFML::Init(window)) {
    return 1;
  }

  sf::Clock deltaClock;
  while (window.isOpen()) {
    while (auto event = window.pollEvent()) {
      ImGui::SFML::ProcessEvent(window, *event);
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
    }

    ImGui::SFML::Update(window, deltaClock.restart());
    window.clear(sf::Color(50, 50, 50));

    // AQUI SE DIBUJA EL MAPA CON SFML (Circulos, lineas, etc)

    ImGui::SFML::Render(window);
    window.display();
  }

  ImGui::SFML::Shutdown();
  return 0;
}