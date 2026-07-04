#pragma once
#include <string>
#include <vector>

// ─── Productos ────────────────────────────────────────────────────────────────
struct Producto {
    std::string nombre;
    float       peso;   // kg
};

// Suma el peso total de productos de un nodo
inline float demandaTotal(const std::vector<Producto>& prods) {
    float t = 0.f;
    for (const auto& p : prods) t += p.peso;
    return t;
}

// ─── Grafo de calles ──────────────────────────────────────────────────────────
struct NodoGrafo {
    int   id;
    float x, y;
    std::string nombre;
    bool  esAvenida;
};

struct Arista {
    int   desde, hasta;
    float metros;
    bool  esAvenida;
};

struct Grafo {
    std::vector<NodoGrafo>                             nodos;
    std::vector<Arista>                                aristas;
    std::vector<std::vector<std::pair<int, float>>>    adj;
};

using MatrizDist    = std::vector<std::vector<float>>;
using MatrizCaminos = std::vector<std::vector<std::vector<int>>>;

struct InfoRutas {
    MatrizDist    distancias;
    MatrizCaminos caminos;
};

// ─── Nodos VRP ────────────────────────────────────────────────────────────────
struct Nodo {
    int   id;
    std::string nombre;
    float pos_x, pos_y;
    std::vector<Producto> productos;  // reemplaza float demanda
    bool  esDeposito;
    int   grafoNodeId;
    int   vehiculoAsignado = -1; // -1 = Auto, 0..N = id vehiculo
};

// ─── Vehículos — todos salen del depósito, sin inicio_x/y ────────────────────
struct Vehiculo {
    int   id;
    std::string placa;
    float capacidad;
    float carga_actual;
};

struct ResultadoAlgoritmo {
    std::vector<std::vector<int>> rutas;
    double distanciaTotal;
    double tiempoMs;
};
