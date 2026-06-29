#include "Algoritmos.hpp"
#include <cmath>
#include <chrono>
#include <algorithm>
#include <limits>

static float distM(const Nodo& a, const Nodo& b) {
    return std::abs(a.pos_x - b.pos_x) + std::abs(a.pos_y - b.pos_y);
}

ResultadoAlgoritmo ejecutarFuerzaBruta(const std::vector<Nodo>& nodos, const std::vector<Vehiculo>& vehiculos) {
    auto t0 = std::chrono::high_resolution_clock::now();

    ResultadoAlgoritmo mejor;
    mejor.distanciaTotal = std::numeric_limits<double>::max();

    int depIdx = 0;
    for (int i = 0; i < (int)nodos.size(); ++i)
        if (nodos[i].esDeposito) { depIdx = i; break; }

    std::vector<int> clienteIdxs;
    for (int i = 0; i < (int)nodos.size(); ++i)
        if (!nodos[i].esDeposito) clienteIdxs.push_back(i);

    if (clienteIdxs.empty() || vehiculos.empty()) {
        mejor.distanciaTotal = 0.0;
        auto t1 = std::chrono::high_resolution_clock::now();
        mejor.tiempoMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        return mejor;
    }

    std::sort(clienteIdxs.begin(), clienteIdxs.end());

    do {
        std::vector<std::vector<int>> rutasAct;
        double distAct = 0.0;
        size_t ci = 0;

        for (size_t vi = 0; vi < vehiculos.size() && ci < clienteIdxs.size(); ++vi) {
            std::vector<int> ruta;
            ruta.push_back(depIdx);
            float carga  = vehiculos[vi].capacidad;
            int   actual = depIdx;

            while (ci < clienteIdxs.size() && nodos[clienteIdxs[ci]].demanda <= carga) {
                int nIdx = clienteIdxs[ci];
                distAct += distM(nodos[actual], nodos[nIdx]);
                ruta.push_back(nIdx);
                carga  -= nodos[nIdx].demanda;
                actual  = nIdx;
                ++ci;
            }

            distAct += distM(nodos[actual], nodos[depIdx]);
            ruta.push_back(depIdx);
            rutasAct.push_back(ruta);
        }

        bool valido = (ci >= clienteIdxs.size());

        if (valido && distAct < mejor.distanciaTotal) {
            mejor.distanciaTotal = distAct;
            mejor.rutas = rutasAct;
        }

    } while (std::next_permutation(clienteIdxs.begin(), clienteIdxs.end()));

    auto t1 = std::chrono::high_resolution_clock::now();
    mejor.tiempoMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return mejor;
}
