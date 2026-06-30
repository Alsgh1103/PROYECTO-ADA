#include "Algoritmos.hpp"
#include <chrono>
#include <limits>

// Greedy nearest-neighbor VRP.
// Usa demandaTotal() para calcular la carga real de cada nodo (suma de productos).
ResultadoAlgoritmo ejecutarGreedy(
    const MatrizDist& dist,
    const std::vector<Nodo>& nodos,
    const std::vector<Vehiculo>& vehiculos)
{
    auto t0 = std::chrono::high_resolution_clock::now();

    ResultadoAlgoritmo res;
    res.distanciaTotal = 0.0;

    int depIdx = 0;
    for (int i = 0; i < static_cast<int>(nodos.size()); ++i)
        if (nodos[i].esDeposito) { depIdx = i; break; }

    std::vector<bool> visitado(nodos.size(), false);
    visitado[depIdx] = true;

    for (size_t vi = 0; vi < vehiculos.size(); ++vi) {
        const auto& v = vehiculos[vi];
        std::vector<int> ruta;
        ruta.push_back(depIdx);
        float carga   = v.capacidad;
        int   actual  = depIdx;
        bool  encontro = true;

        while (encontro) {
            encontro = false;
            int   mejorIdx  = -1;
            float mejorDist = std::numeric_limits<float>::max();
            for (int i = 0; i < static_cast<int>(nodos.size()); ++i) {
                if (!visitado[i] && demandaTotal(nodos[i].productos) <= carga) {
                    if (nodos[i].vehiculoAsignado != -1 && nodos[i].vehiculoAsignado != (int)vi) continue;
                    float d = dist[actual][i];
                    if (d < mejorDist) { mejorDist = d; mejorIdx = i; }
                }
            }
            if (mejorIdx >= 0) {
                visitado[mejorIdx] = true;
                carga -= demandaTotal(nodos[mejorIdx].productos);
                res.distanciaTotal += mejorDist;
                ruta.push_back(mejorIdx);
                actual = mejorIdx;
                encontro = true;
            }
        }

        res.distanciaTotal += dist[actual][depIdx];
        ruta.push_back(depIdx);
        res.rutas.push_back(ruta);
    }

    for (int i = 0; i < static_cast<int>(nodos.size()); ++i) {
        if (!visitado[i]) {
            res.distanciaTotal += 1e9;
            break;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    res.tiempoMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
}
