#pragma once
#include "Estructuras.hpp"
#include <cmath>
#include <limits>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  Grafo urbano 16×12 = 192 intersecciones, ~370 aristas + 12 diagonales
//  Área: 3 000 m × 2 200 m  (ciudad mucho más grande y densa)
//  Avenidas principales H: filas 2,5,8,11
//  Avenidas principales V: cols 3,7,12,15
// ─────────────────────────────────────────────────────────────────────────────

static constexpr float MUNDO_MAX_X = 3000.f;   // 15 col-gaps × 200 m
static constexpr float MUNDO_MAX_Y = 2200.f;   // 11 row-gaps × 200 m

// Depósito por defecto: fila=5, col=7 → id=87, pos=(1400,1000)
static constexpr int DEP_GRAFO_ID = 5 * 16 + 7;  // 87

inline bool esAvH(int r) { return r == 2 || r == 5 || r == 8 || r == 11; }
inline bool esAvV(int c) { return c == 3 || c == 7 || c == 12 || c == 15; }

inline Grafo construirMapaUrbano() {
    // 12 calles horizontales
    static const char* CH[12] = {
        "Jr. Ancash",   "Jr. Lampa",    "Av. Grau",          "Jr. Ica",
        "Jr. Ayacucho", "Av. Arequipa", "Jr. Moquegua",      "Jr. Huancavelica",
        "Av. J. Prado", "Jr. Surco",    "Jr. Chorrillos",    "Av. Circunvalacion"
    };
    // 16 calles verticales
    static const char* CV[16] = {
        "Jr. Callao",      "Jr. R. Torrico", "Jr. Montevideo", "Av. Brasil",
        "Jr. Colmena",     "Jr. Abancay",    "Jr. Huallaga",   "Av. Tacna",
        "Jr. Carabaya",    "Jr. Camana",     "Jr. Ocona",      "Jr. De la Union",
        "Av. Wilson",      "Jr. Quilca",     "Jr. Puno",       "Av. Aviacion"
    };

    const int   COLS = 16, ROWS = 12;
    const float PASO = 200.f;

    Grafo g;
    g.nodos.reserve(COLS * ROWS);

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            NodoGrafo n;
            n.id        = r * COLS + c;
            n.x         = float(c) * PASO;
            n.y         = float(r) * PASO;
            n.esAvenida = esAvH(r) || esAvV(c);
            n.nombre    = std::string(CH[r]) + " & " + CV[c];
            g.nodos.push_back(n);
        }
    }

    g.adj.resize(g.nodos.size());

    auto addArista = [&](int a, int b) {
        if (a < 0 || b < 0 ||
            a >= (int)g.nodos.size() ||
            b >= (int)g.nodos.size()) return;
        float dx = g.nodos[b].x - g.nodos[a].x;
        float dy = g.nodos[b].y - g.nodos[a].y;
        float m  = std::sqrt(dx * dx + dy * dy);
        bool  av = g.nodos[a].esAvenida || g.nodos[b].esAvenida;
        g.aristas.push_back({a, b, m, av});
        g.adj[a].push_back({b, m});
        g.adj[b].push_back({a, m});
    };

    // ── Conexiones horizontales ──────────────────────────────────────────────
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS - 1; ++c)
            addArista(r * COLS + c, r * COLS + c + 1);

    // ── Conexiones verticales ────────────────────────────────────────────────
    for (int c = 0; c < COLS; ++c)
        for (int r = 0; r < ROWS - 1; ++r)
            addArista(r * COLS + c, (r + 1) * COLS + c);

    // ── 12 Diagonales para atajos urbanos ────────────────────────────────────
    // (dan carácter orgánico al mapa, como en imagen de referencia)
    const int N = COLS;
    addArista(0*N+0,   2*N+2);    // 1. Esquina NW
    addArista(0*N+13,  2*N+15);   // 2. Esquina NE
    addArista(10*N+0,  8*N+2);    // 3. Esquina SW
    addArista(10*N+13, 8*N+15);   // 4. Esquina SE
    addArista(2*N+3,   4*N+5);    // 5. Centro-Oeste alto
    addArista(8*N+3,   6*N+5);    // 6. Centro-Oeste bajo
    addArista(2*N+10,  4*N+12);   // 7. Centro-Este alto
    addArista(8*N+10,  6*N+12);   // 8. Centro-Este bajo
    addArista(5*N+4,   7*N+6);    // 9. Centro-medio
    addArista(5*N+9,   3*N+11);   // 10. Centro-arriba
    addArista(4*N+1,   2*N+3);    // 11. Lateral-N izq
    addArista(9*N+1,   11*N+3);   // 12. Lateral-S izq

    return g;
}

// Devuelve índice del NodoGrafo más cercano a (wx, wy)
inline int nodoMasCercano(const Grafo& g, float wx, float wy) {
    int   bestId = 0;
    float bestD2 = std::numeric_limits<float>::max();
    for (int i = 0; i < (int)g.nodos.size(); ++i) {
        float dx = g.nodos[i].x - wx;
        float dy = g.nodos[i].y - wy;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestD2) { bestD2 = d2; bestId = i; }
    }
    return bestId;
}
