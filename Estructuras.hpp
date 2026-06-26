#pragma once
#include <vector>
struct Nodo {
  int id;
  float pos_x;
  float pos_y;
};

struct Vehiculo {
  int id;
  float capacidad;
  float carga_actual;
};

struct ConfiguracionVRP {
  int num_nodos;
  int num_vehiculos;
  int capacidad_vehiculos;
};
