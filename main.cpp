#include "Algoritmos.hpp"
#include "Estructuras.hpp"
#include "MapaGrafo.hpp"
#include "RutaGrafo.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  Layout
// ─────────────────────────────────────────────────────────────────────────────
static const float MAP_X = 240.f;
static const float MAP_Y = 10.f;
// MUNDO_MAX_X / MUNDO_MAX_Y provienen de MapaGrafo.hpp (3000 / 2200)

static const std::array<sf::Color, 8> SF_COL = {
    sf::Color(255,  80,  60), sf::Color(100, 160, 255),
    sf::Color( 80, 220, 110), sf::Color(255, 200,  20),
    sf::Color(210,  90, 210), sf::Color( 50, 210, 200),
    sf::Color(255, 150,  40), sf::Color(140, 255,  80)
};
static const std::array<ImVec4, 8> IM_COL = {
    ImVec4(1.f,   0.31f, 0.24f, 1.f), ImVec4(0.39f, 0.63f, 1.f,   1.f),
    ImVec4(0.31f, 0.86f, 0.43f, 1.f), ImVec4(1.f,   0.78f, 0.08f, 1.f),
    ImVec4(0.82f, 0.35f, 0.82f, 1.f), ImVec4(0.20f, 0.82f, 0.78f, 1.f),
    ImVec4(1.f,   0.59f, 0.16f, 1.f), ImVec4(0.55f, 1.f,   0.31f, 1.f)
};

// ─────────────────────────────────────────────────────────────────────────────
//  Modos (sin SetInicioVehiculo: todos los vehículos salen del depósito)
// ─────────────────────────────────────────────────────────────────────────────
enum class Modo { Normal, AgregarTienda, MoverDeposito };

// ─────────────────────────────────────────────────────────────────────────────
//  Conversiones mundo ↔ pantalla usando escalas X/Y separadas
// ─────────────────────────────────────────────────────────────────────────────
static sf::Vector2f mundoAScreen(float wx, float wy, float mapW, float mapH) {
    float escX = (mapW - 80.f) / MUNDO_MAX_X;
    float escY = (mapH - 80.f) / MUNDO_MAX_Y;
    return { MAP_X + 40.f + wx * escX,
             MAP_Y + 40.f + wy * escY };
}
static sf::Vector2f screenAMundo(float sx, float sy, float mapW, float mapH) {
    float escX = (mapW - 80.f) / MUNDO_MAX_X;
    float escY = (mapH - 80.f) / MUNDO_MAX_Y;
    return { (sx - MAP_X - 40.f) / escX,
             (sy - MAP_Y - 40.f) / escY };
}
static sf::String toSFML(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Línea gruesa con RectangleShape rotado
// ─────────────────────────────────────────────────────────────────────────────
static void drawLine(sf::RenderWindow& w, sf::Vector2f a, sf::Vector2f b,
                     sf::Color col, float thick) {
    sf::Vector2f dir = b - a;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.5f) return;
    sf::RectangleShape rect(sf::Vector2f(len, thick));
    rect.setOrigin(sf::Vector2f(0.f, thick * 0.5f));
    rect.setPosition(a);
    rect.setFillColor(col);
    float ang = std::atan2(dir.y, dir.x) * 180.f / 3.14159265f;
    rect.setRotation(sf::degrees(ang));
    w.draw(rect);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Guardar / Cargar tiendas (formato VRP_TIENDAS_V2)
// ─────────────────────────────────────────────────────────────────────────────
static void guardarTiendas(const std::vector<Nodo>& nodos, const char* filename) {
    std::ofstream f(filename);
    if (!f.is_open()) throw std::runtime_error("No se pudo crear el archivo");
    f << "VRP_TIENDAS_V2\n";
    // Depósito
    const auto& dep = nodos[0];
    f << "DEP;" << dep.nombre << ";" << dep.grafoNodeId << ";"
      << dep.pos_x << ";" << dep.pos_y << "\n";
    // Tiendas
    for (size_t i = 1; i < nodos.size(); ++i) {
        const auto& n = nodos[i];
        f << "T;" << n.nombre << ";" << n.grafoNodeId << ";"
          << n.pos_x << ";" << n.pos_y << ";" << n.vehiculoAsignado;
        for (const auto& p : n.productos)
            f << ";" << p.nombre << ";" << p.peso;
        f << "\n";
    }
}

static bool cargarTiendas(std::vector<Nodo>& nodos, const Grafo& grafo,
                           const char* filename) {
    std::ifstream f(filename);
    if (!f.is_open()) return false;
    std::string line;
    if (!std::getline(f, line) || line != "VRP_TIENDAS_V2") return false;

    std::vector<Nodo> loaded;
    if (!nodos.empty()) loaded.push_back(nodos[0]); // conservar depósito actual

    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> parts;
        {
            std::stringstream ss(line);
            std::string p;
            while (std::getline(ss, p, ';')) parts.push_back(p);
        }
        if (parts.empty()) continue;

        if (parts[0] == "DEP" && parts.size() >= 5) {
            int gId = std::stoi(parts[2]);
            if (gId >= 0 && gId < (int)grafo.nodos.size()) {
                loaded[0].nombre      = parts[1];
                loaded[0].grafoNodeId = gId;
                loaded[0].pos_x       = std::stof(parts[3]);
                loaded[0].pos_y       = std::stof(parts[4]);
            }
        } else if (parts[0] == "T" && parts.size() >= 6) {
            int gId = std::stoi(parts[2]);
            if (gId < 0 || gId >= (int)grafo.nodos.size()) continue;
            Nodo n;
            n.id          = static_cast<int>(loaded.size());
            n.nombre      = parts[1];
            n.grafoNodeId = gId;
            n.pos_x       = std::stof(parts[3]);
            n.pos_y       = std::stof(parts[4]);
            n.vehiculoAsignado = std::stoi(parts[5]);
            n.esDeposito  = false;
            for (size_t k = 6; k + 1 < parts.size(); k += 2) {
                Producto p;
                p.nombre = parts[k];
                p.peso   = std::stof(parts[k + 1]);
                n.productos.push_back(p);
            }
            loaded.push_back(n);
        }
    }
    nodos = loaded;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Dibuja el grafo de calles (fondo + calles + parque + intersecciones)
// ─────────────────────────────────────────────────────────────────────────────
static void dibujarGrafo(sf::RenderWindow& w, const Grafo& grafo,
                          float mapW, float mapH) {
    // Fondo oscuro
    sf::RectangleShape fondo(sf::Vector2f(mapW, mapH));
    fondo.setPosition(sf::Vector2f(MAP_X, MAP_Y));
    fondo.setFillColor(sf::Color(9, 11, 20));
    w.draw(fondo);

    // Parque (visual, centro-este del nuevo mapa)
    auto pkTL = mundoAScreen(1800.f,  600.f, mapW, mapH);
    auto pkBR = mundoAScreen(2400.f, 1200.f, mapW, mapH);
    sf::RectangleShape park(sf::Vector2f(pkBR.x - pkTL.x, pkBR.y - pkTL.y));
    park.setPosition(pkTL);
    park.setFillColor(sf::Color(12, 40, 16));
    park.setOutlineColor(sf::Color(18, 60, 24));
    park.setOutlineThickness(1.f);
    w.draw(park);

    // Calles
    for (const auto& a : grafo.aristas) {
        auto pA = mundoAScreen(grafo.nodos[a.desde].x, grafo.nodos[a.desde].y, mapW, mapH);
        auto pB = mundoAScreen(grafo.nodos[a.hasta].x, grafo.nodos[a.hasta].y, mapW, mapH);
        if (a.esAvenida) {
            drawLine(w, pA, pB, sf::Color(30, 40, 65),  9.f);
            drawLine(w, pA, pB, sf::Color(44, 58, 92),  6.f);
        } else {
            drawLine(w, pA, pB, sf::Color(16, 22, 36),  5.f);
            drawLine(w, pA, pB, sf::Color(24, 32, 52),  3.f);
        }
    }

    // Puntos de intersección
    for (const auto& n : grafo.nodos) {
        auto  pos = mundoAScreen(n.x, n.y, mapW, mapH);
        float r   = n.esAvenida ? 2.5f : 1.8f;
        sf::CircleShape dot(r);
        dot.setOrigin(sf::Vector2f(r, r));
        dot.setPosition(pos);
        dot.setFillColor(n.esAvenida ? sf::Color(56, 76, 116) : sf::Color(30, 42, 66));
        w.draw(dot);
    }

    // Borde del área de mapa
    sf::RectangleShape borde(sf::Vector2f(mapW - 2.f, mapH - 2.f));
    borde.setPosition(sf::Vector2f(MAP_X + 1.f, MAP_Y + 1.f));
    borde.setFillColor(sf::Color::Transparent);
    borde.setOutlineColor(sf::Color(55, 85, 145));
    borde.setOutlineThickness(2.f);
    w.draw(borde);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Dibuja las rutas calculadas siguiendo los caminos Dijkstra sobre las calles
// ─────────────────────────────────────────────────────────────────────────────
static void dibujarConexiones(sf::RenderWindow& w,
                               const std::vector<Nodo>& nodos,
                               const ResultadoAlgoritmo& res,
                               const InfoRutas& infoRutas,
                               const Grafo& grafo,
                               bool hayResultado,
                               float mapW, float mapH) {
    if (!hayResultado || infoRutas.caminos.empty()) return;

    for (size_t ri = 0; ri < res.rutas.size(); ++ri) {
        sf::Color col = SF_COL[ri % SF_COL.size()];
        const auto& ruta = res.rutas[ri];
        for (size_t k = 1; k < ruta.size(); ++k) {
            int ia = ruta[k - 1], ib = ruta[k];
            if (ia < 0 || ib < 0 ||
                ia >= static_cast<int>(infoRutas.caminos.size()) ||
                ib >= static_cast<int>(infoRutas.caminos[ia].size())) continue;

            const auto& camino = infoRutas.caminos[ia][ib];
            for (size_t p = 1; p < camino.size(); ++p) {
                int gA = camino[p - 1], gB = camino[p];
                if (gA < 0 || gA >= (int)grafo.nodos.size()) continue;
                if (gB < 0 || gB >= (int)grafo.nodos.size()) continue;
                auto posA = mundoAScreen(grafo.nodos[gA].x, grafo.nodos[gA].y, mapW, mapH);
                auto posB = mundoAScreen(grafo.nodos[gB].x, grafo.nodos[gB].y, mapW, mapH);
                // Sombra + línea coloreada
                drawLine(w, posA, posB, sf::Color(0, 0, 0, 80),   5.f);
                drawLine(w, posA, posB, col,                       3.f);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Marcadores: depósito = cuadrado verde, tiendas = círculos amarillos
// ─────────────────────────────────────────────────────────────────────────────
static void dibujarMarcadores(sf::RenderWindow& w,
                               const std::vector<Nodo>& nodos,
                               float mapW, float mapH,
                               const sf::Font& font) {
    int depIdx = 0;
    for (int i = 0; i < (int)nodos.size(); ++i)
        if (nodos[i].esDeposito) { depIdx = i; break; }

    // ── Tiendas: círculos amarillos ──────────────────────────────────────────
    for (size_t i = 0; i < nodos.size(); ++i) {
        if (nodos[i].esDeposito) continue;
        auto  pos = mundoAScreen(nodos[i].pos_x, nodos[i].pos_y, mapW, mapH);
        float r   = 9.f;

        sf::CircleShape outer(r);
        outer.setOrigin(sf::Vector2f(r, r));
        outer.setPosition(pos);
        outer.setFillColor(sf::Color(255, 200, 50));
        outer.setOutlineColor(sf::Color(150, 100, 0));
        outer.setOutlineThickness(2.f);
        w.draw(outer);

        sf::CircleShape inner(3.5f);
        inner.setOrigin(sf::Vector2f(3.5f, 3.5f));
        inner.setPosition(pos);
        inner.setFillColor(sf::Color(110, 60, 0));
        w.draw(inner);

        sf::Text lbl(font, toSFML(nodos[i].nombre), 10);
        lbl.setFillColor(sf::Color(220, 195, 140));
        lbl.setPosition(sf::Vector2f(pos.x + r + 4.f, pos.y - 8.f));
        w.draw(lbl);

        if (nodos[i].vehiculoAsignado != -1) {
            std::string vTxt = "[V" + std::to_string(nodos[i].vehiculoAsignado) + "]";
            sf::Text vLbl(font, toSFML(vTxt), 9);
            vLbl.setFillColor(sf::Color(100, 255, 100));
            vLbl.setPosition(sf::Vector2f(pos.x + r + 4.f, pos.y + 4.f));
            w.draw(vLbl);
        }
    }

    // ── Depósito: cuadrado verde (igual que imagen de referencia) ────────────
    auto depPos = mundoAScreen(nodos[depIdx].pos_x, nodos[depIdx].pos_y, mapW, mapH);
    float dSz   = 13.f;

    // Sombra
    sf::RectangleShape shadow(sf::Vector2f(dSz * 2.f + 4.f, dSz * 2.f + 4.f));
    shadow.setOrigin(sf::Vector2f(dSz + 2.f, dSz + 2.f));
    shadow.setPosition(depPos);
    shadow.setFillColor(sf::Color(0, 0, 0, 100));
    w.draw(shadow);

    sf::RectangleShape depRect(sf::Vector2f(dSz * 2.f, dSz * 2.f));
    depRect.setOrigin(sf::Vector2f(dSz, dSz));
    depRect.setPosition(depPos);
    depRect.setFillColor(sf::Color(45, 210, 75));
    depRect.setOutlineColor(sf::Color(255, 255, 255));
    depRect.setOutlineThickness(2.f);
    w.draw(depRect);

    sf::Text depLbl(font, toSFML(nodos[depIdx].nombre), 11);
    depLbl.setFillColor(sf::Color::White);
    depLbl.setPosition(sf::Vector2f(depPos.x + dSz + 4.f, depPos.y - 8.f));
    w.draw(depLbl);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Overlay de modo activo
// ─────────────────────────────────────────────────────────────────────────────
static void dibujarOverlay(sf::RenderWindow& w, Modo modo,
                             float mapW, const sf::Font& font) {
    if (modo == Modo::Normal) return;

    std::string txt;
    sf::Color   col;
    if (modo == Modo::AgregarTienda) {
        txt = "MODO: Click en el mapa para colocar tienda (snap a interseccion)";
        col = sf::Color(80, 220, 110);
    } else {
        txt = "MODO: Click en el mapa para mover el deposito (snap a interseccion)";
        col = sf::Color(255, 200, 30);
    }

    sf::RectangleShape bg(sf::Vector2f(mapW - 4.f, 24.f));
    bg.setPosition(sf::Vector2f(MAP_X + 2.f, MAP_Y + 2.f));
    bg.setFillColor(sf::Color(0, 0, 0, 170));
    w.draw(bg);

    sf::Text ovl(font, toSFML(txt), 12);
    ovl.setFillColor(col);
    ovl.setPosition(sf::Vector2f(MAP_X + 8.f, MAP_Y + 4.f));
    w.draw(ovl);
}

// ═════════════════════════════════════════════════════════════════════════════
int main() {
    // ── Construir grafo urbano ────────────────────────────────────────────────
    Grafo grafo = construirMapaUrbano();

    // ── Ventana ───────────────────────────────────────────────────────────────
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1380u, 760u)),
                            "VRP - Logistica GRUPO 2");
    window.setFramerateLimit(60);

    // ── ImGui ─────────────────────────────────────────────────────────────────
    static ImFontAtlas sharedAtlas;
    {
        ImFontConfig cfg;
        cfg.OversampleH = 2; cfg.OversampleV = 2;
        static const ImWchar ranges[] = { 0x0020, 0x024F, 0 };
        ImFont* fnt = sharedAtlas.AddFontFromFileTTF(
            "C:/Windows/Fonts/arial.ttf", 14.f, &cfg, ranges);
        if (!fnt) sharedAtlas.AddFontDefault();
    }
    ImGui::CreateContext(&sharedAtlas);
    if (!ImGui::SFML::Init(window)) return 1;

    auto& sty = ImGui::GetStyle();
    sty.WindowRounding = 6.f; sty.FrameRounding = 4.f; sty.GrabRounding = 4.f;
    sty.Colors[ImGuiCol_WindowBg]       = ImVec4(0.06f, 0.08f, 0.12f, 1.f);
    sty.Colors[ImGuiCol_Header]         = ImVec4(0.15f, 0.26f, 0.50f, 1.f);
    sty.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.25f, 0.40f, 0.70f, 1.f);
    sty.Colors[ImGuiCol_Button]         = ImVec4(0.14f, 0.30f, 0.58f, 1.f);
    sty.Colors[ImGuiCol_ButtonHovered]  = ImVec4(0.22f, 0.44f, 0.78f, 1.f);
    sty.Colors[ImGuiCol_FrameBg]        = ImVec4(0.10f, 0.12f, 0.18f, 1.f);
    sty.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.08f, 0.18f, 0.38f, 1.f);
    sty.Colors[ImGuiCol_PopupBg]        = ImVec4(0.08f, 0.10f, 0.15f, 1.f);
    sty.Colors[ImGuiCol_Separator]      = ImVec4(0.22f, 0.30f, 0.46f, 1.f);
    sty.Colors[ImGuiCol_CheckMark]      = ImVec4(0.4f,  0.8f,  1.f,   1.f);
    sty.Colors[ImGuiCol_SliderGrab]     = ImVec4(0.3f,  0.6f,  1.f,   1.f);

    sf::Font font;
    (void)font.openFromFile("C:/Windows/Fonts/arial.ttf");

    // ── Estado VRP ────────────────────────────────────────────────────────────
    // Depósito inicial: centro del mapa 16×12 → fila=5, col=7, id=87 → (1400m, 1000m)
    std::vector<Nodo> nodos;
    {
        int gId = DEP_GRAFO_ID; // 87, desde MapaGrafo.hpp
        Nodo dep;
        dep.id = 0; dep.nombre = "Deposito";
        dep.pos_x = grafo.nodos[gId].x;
        dep.pos_y = grafo.nodos[gId].y;
        dep.esDeposito = true;
        dep.grafoNodeId = gId;
        nodos.push_back(dep);
    }

    std::vector<Vehiculo> vehiculos;
    ResultadoAlgoritmo    resultado;
    InfoRutas             infoRutas;
    bool hayResultado    = false;
    bool infoActualizada = false;

    // Inputs de vehículo
    char  bufPlaca[16] = {};
    float bufCap = 20.f;

    // Velocidad
    float velocidadKmh = 30.f;

    // Modo activo
    Modo modo = Modo::Normal;

    // Popup de tienda
    bool  esperandoPopup = false;
    float clickWx = 0.f, clickWy = 0.f;
    int   clickGrafoId = 0;
    char  bufNomTienda[64] = {};

    // Popup: productos temporales
    std::vector<Producto> tmpProductos;
    char  bufProdNom[64] = {};
    float bufProdPeso = 10.f;

    // Guardar / Cargar
    char        bufArchivo[256] = "tiendas.vrp";
    std::string msgIO;

    // Algoritmo
    const char* algos[] = { "Greedy", "Fuerza Bruta", "Algoritmo Genetico" };
    int algoSel = 0;

    sf::Clock deltaClock;

    // ── Bucle principal ───────────────────────────────────────────────────────
    while (window.isOpen()) {
        float mapW = static_cast<float>(window.getSize().x) - MAP_X - 10.f;
        float mapH = static_cast<float>(window.getSize().y) - MAP_Y - 10.f;

        // ── Eventos ───────────────────────────────────────────────────────────
        while (auto ev = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *ev);

            if (ev->is<sf::Event::Closed>()) window.close();

            if (modo != Modo::Normal && !esperandoPopup) {
                if (const auto* mp = ev->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mp->button == sf::Mouse::Button::Left &&
                        !ImGui::GetIO().WantCaptureMouse) {
                        float sx = static_cast<float>(mp->position.x);
                        float sy = static_cast<float>(mp->position.y);
                        bool enMapa = sx > MAP_X && sx < MAP_X + mapW &&
                                      sy > MAP_Y && sy < MAP_Y + mapH;
                        if (enMapa) {
                            auto m = screenAMundo(sx, sy, mapW, mapH);

                            if (modo == Modo::AgregarTienda) {
                                int gId = nodoMasCercano(grafo, m.x, m.y);
                                clickGrafoId = gId;
                                clickWx = grafo.nodos[gId].x;
                                clickWy = grafo.nodos[gId].y;
                                bufNomTienda[0] = '\0';
                                tmpProductos.clear();
                                bufProdNom[0] = '\0';
                                bufProdPeso = 1.f;
                                esperandoPopup = true;

                            } else if (modo == Modo::MoverDeposito) {
                                int gId = nodoMasCercano(grafo, m.x, m.y);
                                nodos[0].grafoNodeId = gId;
                                nodos[0].pos_x = grafo.nodos[gId].x;
                                nodos[0].pos_y = grafo.nodos[gId].y;
                                hayResultado    = false;
                                infoActualizada = false;
                                modo = Modo::Normal;
                            }
                        }
                    }
                }
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // ── Popup: Nueva Tienda con productos ─────────────────────────────────
        if (esperandoPopup)
            ImGui::OpenPopup("Nueva Tienda##popup");

        if (ImGui::BeginPopupModal("Nueva Tienda##popup", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.50f, 0.85f, 1.f, 1.f), "Interseccion:");
            ImGui::TextColored(ImVec4(0.85f, 0.82f, 0.60f, 1.f), "%s",
                               grafo.nodos[clickGrafoId].nombre.c_str());
            ImGui::TextColored(ImVec4(0.45f, 0.72f, 0.45f, 1.f),
                               "Pos: (%.0f m, %.0f m)", clickWx, clickWy);
            ImGui::Separator();

            ImGui::PushItemWidth(260);
            ImGui::InputText("Nombre tienda##nt", bufNomTienda, sizeof(bufNomTienda));
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f, 0.88f, 1.f, 1.f), "  Agregar producto:");
            ImGui::Spacing();

            ImGui::PushItemWidth(145);
            ImGui::InputText("Nombre##pn", bufProdNom, sizeof(bufProdNom));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            ImGui::PushItemWidth(72);
            ImGui::InputFloat("kg##pp", &bufProdPeso, 0.f, 0.f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.45f, 0.20f, 1.f));
            if (ImGui::Button("+ Add")) {
                if (bufProdNom[0] != '\0' && bufProdPeso > 0.f) {
                    tmpProductos.push_back({bufProdNom, bufProdPeso});
                    bufProdNom[0] = '\0';
                    bufProdPeso   = 1.f;
                }
            }
            ImGui::PopStyleColor();

            ImGui::Separator();

            float totalDem = 0.f;
            for (int pi = 0; pi < (int)tmpProductos.size(); ++pi) {
                totalDem += tmpProductos[pi].peso;
                ImGui::BulletText("%.1f kg  -  %s",
                                  tmpProductos[pi].peso,
                                  tmpProductos[pi].nombre.c_str());
                ImGui::SameLine();
                ImGui::PushID(pi);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f,0.10f,0.10f,1.f));
                if (ImGui::SmallButton(" X ")) {
                    tmpProductos.erase(tmpProductos.begin() + pi);
                    --pi;
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
            if (tmpProductos.empty())
                ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.45f, 1.f),
                                   "  (sin productos — se puede agregar despues)");

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.f, 0.85f, 0.35f, 1.f),
                               "  Total carga: %.1f kg", totalDem);
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.52f, 0.22f, 1.f));
            if (ImGui::Button("Agregar Tienda", ImVec2(165, 0))) {
                if (bufNomTienda[0] != '\0') {
                    Nodo n;
                    n.id          = static_cast<int>(nodos.size());
                    n.nombre      = bufNomTienda;
                    n.pos_x       = clickWx;
                    n.pos_y       = clickWy;
                    n.productos   = tmpProductos;
                    n.esDeposito  = false;
                    n.grafoNodeId = clickGrafoId;
                    nodos.push_back(n);
                    hayResultado    = false;
                    infoActualizada = false;
                    tmpProductos.clear();
                    modo = Modo::Normal;
                }
                esperandoPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                tmpProductos.clear();
                esperandoPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ── Panel lateral ─────────────────────────────────────────────────────
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(235, static_cast<float>(window.getSize().y)),
                                  ImGuiCond_Always);
        ImGui::Begin("##Panel", nullptr,
            ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize  |
            ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.f, 1.f), "  VRP — Logistica Urbana");
        ImGui::Separator();
        ImGui::Spacing();

        // ─── Vehículos ────────────────────────────────────────────────────────
        if (ImGui::CollapsingHeader("Vehiculos", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(130);
            ImGui::InputText("Placa##vp", bufPlaca, sizeof(bufPlaca));
            ImGui::InputFloat("Cap kg##vc", &bufCap, 5.f, 50.f, "%.0f");
            ImGui::PopItemWidth();
            if (bufCap < 1.f) bufCap = 1.f;

            if (ImGui::Button("Agregar Vehiculo", ImVec2(-1, 0))) {
                if (bufPlaca[0] != '\0') {
                    Vehiculo v;
                    v.id = static_cast<int>(vehiculos.size());
                    v.placa = bufPlaca;
                    v.capacidad = bufCap;
                    v.carga_actual = 0.f;
                    vehiculos.push_back(v);
                    bufPlaca[0] = '\0';
                    hayResultado = false;
                }
            }
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f),
                               "Todos salen del deposito");
            ImGui::Spacing();
            for (size_t i = 0; i < vehiculos.size(); ++i) {
                char bV[128];
                snprintf(bV, sizeof(bV), "V%d  %s  %.0f kg",
                    (int)i, vehiculos[i].placa.c_str(), vehiculos[i].capacidad);
                ImGui::TextColored(IM_COL[i % IM_COL.size()], "%s", bV);
            }
            if (!vehiculos.empty()) {
                ImGui::Spacing();
                if (ImGui::SmallButton("Limpiar vehiculos")) {
                    vehiculos.clear(); hayResultado = false;
                    for(auto& n : nodos) n.vehiculoAsignado = -1;
                }
            }
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // ─── Tiendas ──────────────────────────────────────────────────────────
        if (ImGui::CollapsingHeader("Tiendas", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool modoAgregar  = (modo == Modo::AgregarTienda);
            bool modoMoverDep = (modo == Modo::MoverDeposito);

            // Botón agregar tienda
            if (modoAgregar) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f,0.08f,0.08f,1.f));
                if (ImGui::Button("Cancelar click", ImVec2(-1, 0))) modo = Modo::Normal;
                ImGui::PopStyleColor();
                ImGui::TextColored(ImVec4(1.f,0.88f,0.2f,1.f), "  Click en el mapa...");
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f,0.44f,0.18f,1.f));
                if (ImGui::Button("+ Agregar Tienda (click)", ImVec2(-1, 0)))
                    modo = Modo::AgregarTienda;
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            // Botón mover depósito
            if (modoMoverDep) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f,0.38f,0.05f,1.f));
                if (ImGui::Button("Cancelar mover", ImVec2(-1, 0))) modo = Modo::Normal;
                ImGui::PopStyleColor();
                ImGui::TextColored(ImVec4(1.f,0.88f,0.2f,1.f), "  Click = nueva pos.");
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.38f,0.28f,0.05f,1.f));
                if (ImGui::Button("Mover Deposito (click)", ImVec2(-1, 0)))
                    modo = Modo::MoverDeposito;
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            // Depósito
            ImGui::TextColored(ImVec4(0.30f,0.90f,0.42f,1.f),
                               "[DEP] %s", nodos[0].nombre.c_str());
            ImGui::TextColored(ImVec4(0.45f,0.62f,0.40f,1.f),
                               "  %s", grafo.nodos[nodos[0].grafoNodeId].nombre.c_str());

            // Lista de tiendas
            for (size_t i = 1; i < nodos.size(); ++i) {
                float td = demandaTotal(nodos[i].productos);
                char bT[128];
                snprintf(bT, sizeof(bT), "[T%d] %s - %.0f kg (%d prod)",
                    (int)i, nodos[i].nombre.c_str(), td,
                    (int)nodos[i].productos.size());
                ImGui::TextColored(ImVec4(0.85f,0.75f,0.45f,1.f), "%s", bT);
            }
            if (nodos.size() > 1) {
                ImGui::Spacing();
                if (ImGui::SmallButton("Limpiar tiendas")) {
                    nodos.resize(1);
                    hayResultado = false; infoActualizada = false;
                }
            }

            // ── Guardar / Cargar ──────────────────────────────────────────────
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.65f,0.70f,0.90f,1.f), "Guardar / Cargar:");
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##arc", bufArchivo, sizeof(bufArchivo));
            ImGui::PopItemWidth();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f,0.28f,0.55f,1.f));
            if (ImGui::Button("Guardar Tiendas", ImVec2(-1, 0))) {
                try {
                    guardarTiendas(nodos, bufArchivo);
                    msgIO = std::string("OK: ") + bufArchivo;
                } catch (const std::exception& e) {
                    msgIO = std::string("ERR: ") + e.what();
                }
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f,0.38f,0.20f,1.f));
            if (ImGui::Button("Cargar Tiendas", ImVec2(-1, 0))) {
                if (cargarTiendas(nodos, grafo, bufArchivo)) {
                    msgIO = "OK: " + std::to_string((int)nodos.size()-1) + " tiendas";
                    hayResultado = false; infoActualizada = false;
                } else {
                    msgIO = "ERR: archivo no encontrado";
                }
            }
            ImGui::PopStyleColor();

            if (!msgIO.empty()) {
                bool ok = msgIO.size() >= 2 && msgIO[0]=='O' && msgIO[1]=='K';
                ImGui::TextColored(ok ? ImVec4(0.4f,1.f,0.5f,1.f)
                                      : ImVec4(1.f,0.4f,0.4f,1.f),
                                   "%s", msgIO.c_str());
            }
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // ─── Asignación de Rutas ──────────────────────────────────────────────
        if (ImGui::CollapsingHeader("Asignacion de Rutas", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "Fuerza un vehiculo a visitar ciertas tiendas:");
            ImGui::Spacing();
            for (size_t v = 0; v < vehiculos.size(); ++v) {
                std::string vName = "Vehiculo V" + std::to_string(v) + " (" + vehiculos[v].placa + ")";
                if (ImGui::TreeNode(vName.c_str())) {
                    for (size_t i = 1; i < nodos.size(); ++i) {
                        bool asignado = (nodos[i].vehiculoAsignado == (int)v);
                        std::string chkName = nodos[i].nombre + "##" + std::to_string(v) + "_" + std::to_string(i);
                        
                        if (nodos[i].vehiculoAsignado != -1 && nodos[i].vehiculoAsignado != (int)v) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.5f,1.f));
                            if (ImGui::Checkbox(chkName.c_str(), &asignado)) {
                                if (asignado) nodos[i].vehiculoAsignado = (int)v;
                                hayResultado = false;
                            }
                            ImGui::SameLine();
                            ImGui::Text("(En V%d)", nodos[i].vehiculoAsignado);
                            ImGui::PopStyleColor();
                        } else {
                            if (ImGui::Checkbox(chkName.c_str(), &asignado)) {
                                if (asignado) nodos[i].vehiculoAsignado = (int)v;
                                else nodos[i].vehiculoAsignado = -1;
                                hayResultado = false;
                            }
                        }
                    }
                    if (nodos.size() <= 1) {
                        ImGui::TextColored(ImVec4(0.4f,0.4f,0.4f,1.f), "  (No hay tiendas)");
                    }
                    ImGui::TreePop();
                }
            }
            if (vehiculos.empty()) {
                ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.f), "Agrega vehiculos primero.");
            }
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // ─── Velocidad ────────────────────────────────────────────────────────
        ImGui::TextColored(ImVec4(0.60f,0.85f,1.f,1.f), "Velocidad promedio:");
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat("##vel", &velocidadKmh, 10.f, 120.f, "%.0f km/h");
        ImGui::PopItemWidth();

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // ─── Algoritmo ────────────────────────────────────────────────────────
        ImGui::TextColored(ImVec4(0.60f,0.85f,1.f,1.f), "Algoritmo:");
        ImGui::PushItemWidth(-1);
        ImGui::Combo("##algo", &algoSel, algos, 3);
        ImGui::PopItemWidth();

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // ─── Botón calcular ───────────────────────────────────────────────────
        bool puedeCalc = !vehiculos.empty() && nodos.size() > 1;
        if (!puedeCalc) {
            ImGui::TextColored(ImVec4(1.f,0.50f,0.28f,1.f),
                               "Agrega vehiculos\ny al menos 1 tienda.");
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.08f,0.50f,0.20f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f,0.70f,0.30f,1.f));
            if (ImGui::Button("CALCULAR RUTA", ImVec2(-1, 44))) {
                modo = Modo::Normal;
                // 1. Precomputar distancias reales (Dijkstra)
                infoRutas       = precalcularRutas(grafo, nodos);
                infoActualizada = true;
                // 2. Ejecutar algoritmo VRP elegido
                switch (algoSel) {
                    case 0:  resultado = ejecutarGreedy(infoRutas.distancias, nodos, vehiculos); break;
                    case 1:  resultado = ejecutarFuerzaBruta(infoRutas.distancias, nodos, vehiculos); break;
                    default: resultado = ejecutarGenetico(infoRutas.distancias, nodos, vehiculos); break;
                }
                hayResultado = true;
            }
            ImGui::PopStyleColor(2);
        }

        // ─── Resultados ───────────────────────────────────────────────────────
        if (hayResultado) {
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.35f,1.f,0.50f,1.f), " RESULTADOS");

            if (resultado.distanciaTotal >= 1e9) {
                ImGui::TextColored(ImVec4(1.f,0.3f,0.3f,1.f), "Error: No se encontro una ruta valida.");
                ImGui::TextColored(ImVec4(0.9f,0.9f,0.9f,1.f), "- Verifica que la capacidad sea suficiente.");
                ImGui::TextColored(ImVec4(0.9f,0.9f,0.9f,1.f), "- Verifica asignaciones manuales.");
            } else {
                double kmTot = resultado.distanciaTotal / 1000.0;
                double minTot = (velocidadKmh > 0.f)
                                ? (kmTot / velocidadKmh) * 60.0 : 0.0;

                ImGui::Text("Algoritmo: %s", algos[algoSel]);
                ImGui::Text("Dist total: %.0f m  (%.2f km)", resultado.distanciaTotal, kmTot);
                ImGui::TextColored(ImVec4(0.4f,0.9f,1.f,1.f),
                                   "Viaje total: %.1f min  @%.0f km/h",
                                   minTot, velocidadKmh);
                ImGui::TextColored(ImVec4(0.48f,0.48f,0.48f,1.f),
                                   "Ejecucion alg: %.3f ms", resultado.tiempoMs);

                int nN = static_cast<int>(nodos.size());

                for (size_t ri = 0; ri < resultado.rutas.size(); ++ri) {
                    const auto& ruta = resultado.rutas[ri];
                    if (ruta.size() <= 2) continue; // Skip vehicles that didn't visit any store

                    // Calcular distancia y tiempo de este vehículo
                double dVeh = 0.0;
                for (size_t k = 1; k < ruta.size(); ++k) {
                    int ia = ruta[k-1], ib = ruta[k];
                    if (ia >= 0 && ib >= 0 && ia < nN && ib < nN && infoActualizada)
                        dVeh += infoRutas.distancias[ia][ib];
                }
                double tVeh = (velocidadKmh > 0.f)
                              ? (dVeh / 1000.0 / velocidadKmh) * 60.0 : 0.0;

                ImGui::Separator();
                std::string vLabel = "V" + std::to_string(ri);
                if (ri < vehiculos.size())
                    vLabel += " (" + vehiculos[ri].placa + ")";
                ImGui::TextColored(IM_COL[ri % IM_COL.size()], "%s", vLabel.c_str());

                char bStat[128];
                snprintf(bStat, sizeof(bStat), "  %.0f m | %.1f min", dVeh, tVeh);
                ImGui::TextColored(IM_COL[ri % IM_COL.size()], "%s", bStat);

                // Ruta de paradas
                std::string s;
                for (size_t k = 0; k < ruta.size(); ++k) {
                    int id = ruta[k];
                    if (id >= 0 && id < nN) {
                        s += nodos[id].nombre;
                        if (k + 1 < ruta.size()) s += " > ";
                    }
                }
                ImGui::TextWrapped("%s", s.c_str());
                ImGui::Spacing();
            }
        }
    }

    ImGui::End();

        if (modo != Modo::Normal && !esperandoPopup)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        // ── Render ────────────────────────────────────────────────────────────
        window.clear(sf::Color(7, 9, 16));
        dibujarGrafo(window, grafo, mapW, mapH);
        dibujarConexiones(window, nodos, resultado,
                          infoRutas, grafo,
                          hayResultado && infoActualizada,
                          mapW, mapH);
        dibujarMarcadores(window, nodos, mapW, mapH, font);
        dibujarOverlay(window, modo, mapW, font);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}