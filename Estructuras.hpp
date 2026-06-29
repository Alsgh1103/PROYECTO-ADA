#pragma once
#include <string>
#include <vector>

struct Nodo {
    int id;
    std::string nombre;
    float pos_x;
    float pos_y;
    float demanda;
    bool esDeposito;
};

struct Vehiculo {
    int id;
    std::string placa;
    float capacidad;
    float carga_actual;
    float inicio_x;
    float inicio_y;
    bool  tieneInicio;
};

struct ResultadoAlgoritmo {
    std::vector<std::vector<int>> rutas;
    double distanciaTotal;
    double tiempoMs;
};
