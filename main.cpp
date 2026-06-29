#include "Algoritmos.hpp"
#include "Estructuras.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <array>
#include <cmath>
#include <string>
#include <vector>

static const float MAP_X     = 230.f;
static const float MAP_Y     = 10.f;
static const float MUNDO_MAX = 500.f;

static const std::array<sf::Color, 8> SF_COL = {
    sf::Color(255,  99,  71), sf::Color(100, 149, 237),
    sf::Color( 80, 220, 120), sf::Color(255, 210,   0),
    sf::Color(210, 100, 210), sf::Color( 60, 210, 200),
    sf::Color(255, 160,  50), sf::Color(160, 255,  80)
};

static const std::array<ImVec4, 8> IM_COL = {
    ImVec4(1.f,   0.39f, 0.28f, 1.f), ImVec4(0.39f, 0.58f, 0.93f, 1.f),
    ImVec4(0.31f, 0.86f, 0.47f, 1.f), ImVec4(1.f,   0.82f, 0.f,   1.f),
    ImVec4(0.82f, 0.39f, 0.82f, 1.f), ImVec4(0.24f, 0.82f, 0.78f, 1.f),
    ImVec4(1.f,   0.63f, 0.20f, 1.f), ImVec4(0.63f, 1.f,   0.31f, 1.f)
};

enum class Modo { Normal, AgregarTienda, MoverDeposito, SetInicioVehiculo };

static sf::Vector2f mundoAScreen(float wx, float wy, float mapW, float mapH) {
    return {
        MAP_X + 40.f + (wx / MUNDO_MAX) * (mapW - 80.f),
        MAP_Y + 40.f + (wy / MUNDO_MAX) * (mapH - 80.f)
    };
}

static sf::Vector2f screenAMundo(float sx, float sy, float mapW, float mapH) {
    return {
        ((sx - MAP_X - 40.f) / (mapW - 80.f)) * MUNDO_MAX,
        ((sy - MAP_Y - 40.f) / (mapH - 80.f)) * MUNDO_MAX
    };
}

static sf::String toSFML(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

static void dibujarCiudad(sf::RenderWindow& w, float mapW, float mapH) {
    sf::RectangleShape fondo(sf::Vector2f(mapW, mapH));
    fondo.setPosition(sf::Vector2f(MAP_X, MAP_Y));
    fondo.setFillColor(sf::Color(14, 17, 28));
    w.draw(fondo);

    const int COLS = 12, ROWS = 8;
    float bw = (mapW - 80.f) / COLS;
    float bh = (mapH - 80.f) / ROWS;
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int shade = 22 + ((r + c) % 3) * 4;
            sf::RectangleShape bloque(sf::Vector2f(bw - 6.f, bh - 6.f));
            bloque.setPosition(sf::Vector2f(
                MAP_X + 40.f + c * bw + 3.f,
                MAP_Y + 40.f + r * bh + 3.f));
            bloque.setFillColor(sf::Color(shade, shade + 4, shade + 12));
            bloque.setOutlineColor(sf::Color(38, 48, 68));
            bloque.setOutlineThickness(0.5f);
            w.draw(bloque);
        }
    }

    sf::RectangleShape borde(sf::Vector2f(mapW - 2.f, mapH - 2.f));
    borde.setPosition(sf::Vector2f(MAP_X + 1.f, MAP_Y + 1.f));
    borde.setFillColor(sf::Color::Transparent);
    borde.setOutlineColor(sf::Color(65, 95, 155));
    borde.setOutlineThickness(2.f);
    w.draw(borde);
}

static void dibujarSegL(sf::RenderWindow& w, sf::Vector2f pA, sf::Vector2f pB, sf::Color col) {
    sf::Vector2f esq(pB.x, pA.y);
    sf::Vertex s1[2] = { sf::Vertex{pA,  col}, sf::Vertex{esq, col} };
    sf::Vertex s2[2] = { sf::Vertex{esq, col}, sf::Vertex{pB,  col} };
    w.draw(s1, 2, sf::PrimitiveType::Lines);
    w.draw(s2, 2, sf::PrimitiveType::Lines);
}

static void dibujarConexiones(sf::RenderWindow& w,
                               const std::vector<Nodo>& nodos,
                               const std::vector<Vehiculo>& vehiculos,
                               const ResultadoAlgoritmo& res,
                               bool hayResultado,
                               float mapW, float mapH) {
    int depIdx = 0;
    for (int i = 0; i < (int)nodos.size(); ++i)
        if (nodos[i].esDeposito) { depIdx = i; break; }

    for (size_t vi = 0; vi < vehiculos.size(); ++vi) {
        if (!vehiculos[vi].tieneInicio) continue;
        sf::Color col = SF_COL[vi % SF_COL.size()];
        sf::Color dim(col.r, col.g, col.b, 90);
        auto sPt = mundoAScreen(vehiculos[vi].inicio_x, vehiculos[vi].inicio_y, mapW, mapH);
        auto dPt = mundoAScreen(nodos[depIdx].pos_x, nodos[depIdx].pos_y, mapW, mapH);
        sf::Vector2f esq(dPt.x, sPt.y);
        sf::Vertex l1[2] = { sf::Vertex{sPt, dim}, sf::Vertex{esq, dim} };
        sf::Vertex l2[2] = { sf::Vertex{esq, dim}, sf::Vertex{dPt, dim} };
        w.draw(l1, 2, sf::PrimitiveType::Lines);
        w.draw(l2, 2, sf::PrimitiveType::Lines);
    }

    if (!hayResultado) return;

    for (size_t ri = 0; ri < res.rutas.size(); ++ri) {
        sf::Color col = SF_COL[ri % SF_COL.size()];
        const auto& ruta = res.rutas[ri];
        for (size_t k = 1; k < ruta.size(); ++k) {
            int ia = ruta[k - 1], ib = ruta[k];
            if (ia < 0 || ia >= (int)nodos.size()) continue;
            if (ib < 0 || ib >= (int)nodos.size()) continue;
            auto pA = mundoAScreen(nodos[ia].pos_x, nodos[ia].pos_y, mapW, mapH);
            auto pB = mundoAScreen(nodos[ib].pos_x, nodos[ib].pos_y, mapW, mapH);
            dibujarSegL(w, pA, pB, col);
        }
    }
}

static void dibujarMarcadores(sf::RenderWindow& w,
                               const std::vector<Nodo>& nodos,
                               const std::vector<Vehiculo>& vehiculos,
                               const ResultadoAlgoritmo& res,
                               bool hayResultado,
                               float mapW, float mapH,
                               const sf::Font& font) {
    int depIdx = 0;
    for (int i = 0; i < (int)nodos.size(); ++i)
        if (nodos[i].esDeposito) { depIdx = i; break; }

    for (size_t vi = 0; vi < vehiculos.size(); ++vi) {
        if (!vehiculos[vi].tieneInicio) continue;
        sf::Color col = SF_COL[vi % SF_COL.size()];
        auto pos = mundoAScreen(vehiculos[vi].inicio_x, vehiculos[vi].inicio_y, mapW, mapH);

        sf::ConvexShape diamond;
        diamond.setPointCount(4);
        float sz = 10.f;
        diamond.setPoint(0, sf::Vector2f(0.f,  -sz));
        diamond.setPoint(1, sf::Vector2f(sz,   0.f));
        diamond.setPoint(2, sf::Vector2f(0.f,   sz));
        diamond.setPoint(3, sf::Vector2f(-sz,  0.f));
        diamond.setPosition(pos);
        diamond.setFillColor(col);
        diamond.setOutlineColor(sf::Color::White);
        diamond.setOutlineThickness(1.5f);
        w.draw(diamond);

        std::string lbl = "Inicio V" + std::to_string(vi);
        sf::Text lt(font, toSFML(lbl), 10);
        lt.setFillColor(sf::Color::White);
        lt.setPosition(sf::Vector2f(pos.x + sz + 4.f, pos.y - 8.f));
        w.draw(lt);
    }

    if (hayResultado) {
        auto depPos = mundoAScreen(nodos[depIdx].pos_x, nodos[depIdx].pos_y, mapW, mapH);
        int nV = (int)res.rutas.size();
        for (int ri = 0; ri < nV; ++ri) {
            float ang = (nV == 1) ? -1.5708f : -1.5708f + ri * (6.2832f / nV);
            float ox = 28.f * std::cos(ang);
            float oy = 28.f * std::sin(ang);
            sf::Vector2f mPos(depPos.x + ox, depPos.y + oy);

            sf::CircleShape m(9.f);
            m.setOrigin(sf::Vector2f(9.f, 9.f));
            m.setPosition(mPos);
            m.setFillColor(SF_COL[ri % SF_COL.size()]);
            m.setOutlineColor(sf::Color::White);
            m.setOutlineThickness(1.5f);
            w.draw(m);

            std::string vl = "V" + std::to_string(ri);
            sf::Text vt(font, toSFML(vl), 9);
            vt.setFillColor(sf::Color::White);
            auto bnd = vt.getLocalBounds();
            vt.setOrigin(sf::Vector2f(
                bnd.position.x + bnd.size.x * 0.5f,
                bnd.position.y + bnd.size.y * 0.5f));
            vt.setPosition(mPos);
            w.draw(vt);
        }
    }

    for (const auto& n : nodos) {
        auto pos = mundoAScreen(n.pos_x, n.pos_y, mapW, mapH);
        float r = n.esDeposito ? 13.f : 8.f;
        sf::CircleShape c(r);
        c.setOrigin(sf::Vector2f(r, r));
        c.setPosition(pos);
        c.setFillColor(n.esDeposito ? sf::Color(255, 195, 0) : sf::Color(90, 170, 255));
        c.setOutlineColor(sf::Color(220, 220, 220));
        c.setOutlineThickness(1.5f);
        w.draw(c);

        sf::Text label(font, toSFML(n.nombre), 11);
        label.setFillColor(sf::Color(215, 215, 215));
        label.setPosition(sf::Vector2f(pos.x + r + 4.f, pos.y - 8.f));
        w.draw(label);
    }
}

static void dibujarOverlay(sf::RenderWindow& w, Modo modo, int vehiculoEnFoco,
                             float mapW, const sf::Font& font) {
    if (modo == Modo::Normal) return;

    std::string txt;
    sf::Color col;
    if (modo == Modo::AgregarTienda) {
        txt = "MODO: Click para agregar tienda";
        col = sf::Color(80, 220, 120);
    } else if (modo == Modo::MoverDeposito) {
        txt = "MODO: Click para mover el deposito";
        col = sf::Color(255, 210, 0);
    } else if (modo == Modo::SetInicioVehiculo && vehiculoEnFoco >= 0) {
        txt = "MODO: Click = inicio Vehiculo " + std::to_string(vehiculoEnFoco);
        col = SF_COL[vehiculoEnFoco % SF_COL.size()];
    }

    sf::RectangleShape bg(sf::Vector2f(mapW - 4.f, 24.f));
    bg.setPosition(sf::Vector2f(MAP_X + 2.f, MAP_Y + 2.f));
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    w.draw(bg);

    sf::Text ovl(font, toSFML(txt), 13);
    ovl.setFillColor(col);
    ovl.setPosition(sf::Vector2f(MAP_X + 8.f, MAP_Y + 4.f));
    w.draw(ovl);
}

int main() {
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280u, 720u)), "VRP - Logistica GRUPO 2");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) return 1;

    auto& sty = ImGui::GetStyle();
    sty.WindowRounding = 6.f; sty.FrameRounding = 4.f; sty.GrabRounding = 4.f;
    sty.Colors[ImGuiCol_WindowBg]       = ImVec4(0.07f, 0.09f, 0.13f, 1.f);
    sty.Colors[ImGuiCol_Header]         = ImVec4(0.18f, 0.28f, 0.52f, 1.f);
    sty.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.28f, 0.42f, 0.72f, 1.f);
    sty.Colors[ImGuiCol_Button]         = ImVec4(0.16f, 0.33f, 0.62f, 1.f);
    sty.Colors[ImGuiCol_ButtonHovered]  = ImVec4(0.26f, 0.48f, 0.82f, 1.f);
    sty.Colors[ImGuiCol_FrameBg]        = ImVec4(0.11f, 0.13f, 0.19f, 1.f);
    sty.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.10f, 0.20f, 0.42f, 1.f);
    sty.Colors[ImGuiCol_PopupBg]        = ImVec4(0.09f, 0.11f, 0.17f, 1.f);
    sty.Colors[ImGuiCol_Separator]      = ImVec4(0.25f, 0.32f, 0.48f, 1.f);
    sty.Colors[ImGuiCol_CheckMark]      = ImVec4(0.4f,  0.8f,  1.f,   1.f);

    sf::Font font;
    (void)font.openFromFile("C:/Windows/Fonts/arial.ttf");

    std::vector<Nodo> nodos;
    {
        Nodo dep;
        dep.id = 0; dep.nombre = "Deposito";
        dep.pos_x = 250.f; dep.pos_y = 250.f;
        dep.demanda = 0.f;  dep.esDeposito = true;
        nodos.push_back(dep);
    }

    std::vector<Vehiculo> vehiculos;
    ResultadoAlgoritmo    resultado;
    bool hayResultado = false;

    char  bufPlaca[16] = {};
    float bufCap = 100.f;

    Modo  modo = Modo::Normal;
    int   vehiculoEnFoco = -1;

    bool  esperandoPopup = false;
    float clickWx = 0.f, clickWy = 0.f;
    char  bufNomTienda[64] = {};
    float bufDem = 10.f;

    const char* algos[] = { "Greedy", "Fuerza Bruta", "Algoritmo Genetico" };
    int algoSel = 0;

    sf::Clock deltaClock;

    while (window.isOpen()) {
        float mapW = (float)window.getSize().x - MAP_X - 10.f;
        float mapH = (float)window.getSize().y - MAP_Y - 10.f;

        while (auto ev = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *ev);

            if (ev->is<sf::Event::Closed>())
                window.close();

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
                                clickWx = m.x; clickWy = m.y;
                                bufNomTienda[0] = '\0'; bufDem = 10.f;
                                esperandoPopup = true;

                            } else if (modo == Modo::MoverDeposito) {
                                nodos[0].pos_x = m.x;
                                nodos[0].pos_y = m.y;
                                hayResultado = false;
                                modo = Modo::Normal;

                            } else if (modo == Modo::SetInicioVehiculo &&
                                       vehiculoEnFoco >= 0 &&
                                       vehiculoEnFoco < (int)vehiculos.size()) {
                                vehiculos[vehiculoEnFoco].inicio_x = m.x;
                                vehiculos[vehiculoEnFoco].inicio_y = m.y;
                                vehiculos[vehiculoEnFoco].tieneInicio = true;
                                modo = Modo::Normal;
                                vehiculoEnFoco = -1;
                            }
                        }
                    }
                }
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        if (esperandoPopup)
            ImGui::OpenPopup("Nueva Tienda##popup");

        if (ImGui::BeginPopupModal("Nueva Tienda##popup", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.f, 1.f),
                "Posicion: (%.1f, %.1f)", clickWx, clickWy);
            ImGui::Separator();
            ImGui::PushItemWidth(185);
            ImGui::InputText("Nombre##pop", bufNomTienda, sizeof(bufNomTienda));
            ImGui::InputFloat("Demanda kg##pop", &bufDem, 1.f, 10.f, "%.0f");
            ImGui::PopItemWidth();
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.55f, 0.25f, 1.f));
            if (ImGui::Button("Agregar", ImVec2(135, 0))) {
                if (bufNomTienda[0] != '\0') {
                    Nodo n;
                    n.id = (int)nodos.size();
                    n.nombre    = bufNomTienda;
                    n.pos_x     = clickWx; n.pos_y = clickWy;
                    n.demanda   = bufDem;  n.esDeposito = false;
                    nodos.push_back(n);
                    hayResultado = false;
                }
                esperandoPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(135, 0))) {
                esperandoPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(225, (float)window.getSize().y), ImGuiCond_Always);
        ImGui::Begin("##Panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar);

        ImGui::TextColored(ImVec4(0.38f, 0.78f, 1.f, 1.f), "  VRP - Logistica");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Vehiculos", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(130);
            ImGui::InputText("Placa##v", bufPlaca, sizeof(bufPlaca));
            ImGui::InputFloat("Cap kg##v", &bufCap, 5.f, 50.f, "%.0f");
            ImGui::PopItemWidth();
            if (bufCap < 1.f) bufCap = 1.f;
            if (ImGui::Button("Agregar Vehiculo", ImVec2(-1, 0))) {
                if (bufPlaca[0] != '\0') {
                    Vehiculo v;
                    v.id = (int)vehiculos.size();
                    v.placa = bufPlaca;
                    v.capacidad = bufCap;
                    v.carga_actual = 0.f;
                    v.inicio_x = 0.f; v.inicio_y = 0.f;
                    v.tieneInicio = false;
                    vehiculos.push_back(v);
                    bufPlaca[0] = '\0';
                    hayResultado = false;
                }
            }
            ImGui::Spacing();
            for (size_t i = 0; i < vehiculos.size(); ++i) {
                char bufV[128];
                snprintf(bufV, sizeof(bufV), "[V%d] %s %.0fkg",
                    (int)i, vehiculos[i].placa.c_str(), vehiculos[i].capacidad);
                ImGui::TextColored(IM_COL[i % IM_COL.size()], "%s", bufV);

                ImGui::SameLine();
                bool esActivo = (modo == Modo::SetInicioVehiculo && vehiculoEnFoco == (int)i);
                if (esActivo) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
                ImGui::PushID((int)i);
                if (ImGui::SmallButton("Inicio")) {
                    if (esActivo) {
                        modo = Modo::Normal;
                        vehiculoEnFoco = -1;
                    } else {
                        modo = Modo::SetInicioVehiculo;
                        vehiculoEnFoco = (int)i;
                    }
                }
                ImGui::PopID();
                if (esActivo) ImGui::PopStyleColor();

                if (vehiculos[i].tieneInicio) {
                    ImGui::SameLine();
                    ImGui::TextColored(IM_COL[i % IM_COL.size()], "%s", "(*)");
                }
            }
            if (!vehiculos.empty()) {
                ImGui::Spacing();
                if (ImGui::SmallButton("Limpiar vehiculos")) {
                    vehiculos.clear(); hayResultado = false;
                    modo = Modo::Normal; vehiculoEnFoco = -1;
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Tiendas", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool modoAgregar  = (modo == Modo::AgregarTienda);
            bool modoMoverDep = (modo == Modo::MoverDeposito);

            if (modoAgregar) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.08f, 0.08f, 1.f));
                if (ImGui::Button("Cancelar click", ImVec2(-1, 0))) modo = Modo::Normal;
                ImGui::PopStyleColor();
                ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "  Click en el mapa...");
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.45f, 0.18f, 1.f));
                if (ImGui::Button("+ Agregar Tienda (click)", ImVec2(-1, 0)))
                    modo = Modo::AgregarTienda;
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            if (modoMoverDep) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.40f, 0.05f, 1.f));
                if (ImGui::Button("Cancelar mover", ImVec2(-1, 0))) modo = Modo::Normal;
                ImGui::PopStyleColor();
                ImGui::TextColored(ImVec4(1.f, 0.85f, 0.f, 1.f), "  Click = nueva pos.");
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.30f, 0.05f, 1.f));
                if (ImGui::Button("Mover Deposito (click)", ImVec2(-1, 0)))
                    modo = Modo::MoverDeposito;
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            {
                char bufDep[128];
                snprintf(bufDep, sizeof(bufDep), "[DEP] %s (%.0f,%.0f)",
                    nodos[0].nombre.c_str(), nodos[0].pos_x, nodos[0].pos_y);
                ImGui::TextColored(ImVec4(1.f, 0.8f, 0.f, 1.f), "%s", bufDep);
            }
            for (size_t i = 1; i < nodos.size(); ++i) {
                char bufT[128];
                snprintf(bufT, sizeof(bufT), "[T%d] %s %.0fkg",
                    (int)i, nodos[i].nombre.c_str(), nodos[i].demanda);
                ImGui::Text("%s", bufT);
            }
            if (nodos.size() > 1) {
                ImGui::Spacing();
                if (ImGui::SmallButton("Limpiar tiendas")) {
                    nodos.resize(1); hayResultado = false;
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Algoritmo", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            ImGui::Combo("##algo", &algoSel, algos, 3);
            ImGui::PopItemWidth();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool puedeCalc = !vehiculos.empty() && nodos.size() > 1;
        if (!puedeCalc) {
            ImGui::TextColored(ImVec4(1.f, 0.5f, 0.3f, 1.f),
                "Necesitas vehiculos\ny al menos 1 tienda.");
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.08f, 0.52f, 0.22f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.72f, 0.32f, 1.f));
            if (ImGui::Button("CALCULAR RUTA", ImVec2(-1, 42))) {
                modo = Modo::Normal; vehiculoEnFoco = -1;
                switch (algoSel) {
                    case 0:  resultado = ejecutarGreedy(nodos, vehiculos);      break;
                    case 1:  resultado = ejecutarFuerzaBruta(nodos, vehiculos); break;
                    default: resultado = ejecutarGenetico(nodos, vehiculos);    break;
                }
                hayResultado = true;
            }
            ImGui::PopStyleColor(2);
        }

        if (hayResultado) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.38f, 1.f, 0.55f, 1.f), "%s", "RESULTADOS");
            {
                char bA[64], bD[64], bT[64];
                snprintf(bA, sizeof(bA), "Algoritmo: %s", algos[algoSel]);
                snprintf(bD, sizeof(bD), "Distancia: %.2f u", resultado.distanciaTotal);
                snprintf(bT, sizeof(bT), "Tiempo:    %.3f ms", resultado.tiempoMs);
                ImGui::Text("%s", bA);
                ImGui::Text("%s", bD);
                ImGui::Text("%s", bT);
            }
            ImGui::Spacing();
            for (size_t ri = 0; ri < resultado.rutas.size(); ++ri) {
                char bV[16];
                snprintf(bV, sizeof(bV), "V%d:", (int)ri);
                ImGui::TextColored(IM_COL[ri % IM_COL.size()], "%s", bV);
                const auto& ruta = resultado.rutas[ri];
                std::string s;
                for (size_t k = 0; k < ruta.size(); ++k) {
                    int id = ruta[k];
                    if (id >= 0 && id < (int)nodos.size()) {
                        s += nodos[id].nombre;
                        if (k + 1 < ruta.size()) s += " > ";
                    }
                }
                ImGui::TextWrapped("%s", s.c_str());
                ImGui::Spacing();
            }
        }

        ImGui::End();

        if (modo != Modo::Normal && !esperandoPopup)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        window.clear(sf::Color(8, 10, 18));
        dibujarCiudad(window, mapW, mapH);
        dibujarConexiones(window, nodos, vehiculos, resultado, hayResultado, mapW, mapH);
        dibujarMarcadores(window, nodos, vehiculos, resultado, hayResultado, mapW, mapH, font);
        dibujarOverlay(window, modo, vehiculoEnFoco, mapW, font);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}