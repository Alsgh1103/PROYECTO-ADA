#include "Algoritmos.hpp"
#include <cmath>
#include <chrono>
#include <limits>

static float distM(const Nodo& a, const Nodo& b) {
    return std::abs(a.pos_x - b.pos_x) + std::abs(a.pos_y - b.pos_y);
}

ResultadoAlgoritmo ejecutarGreedy(const std::vector<Nodo>& nodos, const std::vector<Vehiculo>& vehiculos) {
    auto t0 = std::chrono::high_resolution_clock::now();

    ResultadoAlgoritmo res;
    res.distanciaTotal = 0.0;

    int depIdx = 0;
    for (int i = 0; i < (int)nodos.size(); ++i)
        if (nodos[i].esDeposito) { depIdx = i; break; }

    std::vector<bool> visitado(nodos.size(), false);
    visitado[depIdx] = true;

    for (const auto& v : vehiculos) {
        std::vector<int> ruta;
        ruta.push_back(depIdx);
        float carga = v.capacidad;
        int actual  = depIdx;
        bool encontro = true;

        while (encontro) {
            encontro = false;
            int   mejorIdx  = -1;
            float mejorDist = std::numeric_limits<float>::max();
            for (int i = 0; i < (int)nodos.size(); ++i) {
                if (!visitado[i] && nodos[i].demanda <= carga) {
                    float d = distM(nodos[actual], nodos[i]);
                    if (d < mejorDist) { mejorDist = d; mejorIdx = i; }
                }
            }
            if (mejorIdx >= 0) {
                visitado[mejorIdx] = true;
                carga -= nodos[mejorIdx].demanda;
                res.distanciaTotal += mejorDist;
                ruta.push_back(mejorIdx);
                actual = mejorIdx;
                encontro = true;
            }
        }

        res.distanciaTotal += distM(nodos[actual], nodos[depIdx]);
        ruta.push_back(depIdx);
        res.rutas.push_back(ruta);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    res.tiempoMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
}
