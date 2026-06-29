#pragma once
#include <vector>
#include "Estructuras.hpp"

ResultadoAlgoritmo ejecutarGreedy(const std::vector<Nodo>& nodos, const std::vector<Vehiculo>& vehiculos);
ResultadoAlgoritmo ejecutarFuerzaBruta(const std::vector<Nodo>& nodos, const std::vector<Vehiculo>& vehiculos);
ResultadoAlgoritmo ejecutarGenetico(const std::vector<Nodo>& nodos, const std::vector<Vehiculo>& vehiculos);
