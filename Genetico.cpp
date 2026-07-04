#include "Algoritmos.hpp"
#include <algorithm>
#include <chrono>
#include <limits>
#include <random>

using Crom = std::vector<int>;

// Evaluación de fitness. Usa demandaTotal() para validar capacidad.
static double evaluar(const Crom& c,
                      const MatrizDist& dist,
                      const std::vector<Nodo>& nodos,
                      const std::vector<Vehiculo>& vehiculos,
                      int depIdx,
                      std::vector<std::vector<int>>& rutasSalida)
{
    rutasSalida.clear();
    double d  = 0.0;
    size_t ci = 0;

    for (size_t vi = 0; vi < vehiculos.size() && ci < c.size(); ++vi) {
        std::vector<int> ruta;
        ruta.push_back(depIdx);
        float carga  = vehiculos[vi].capacidad;
        int   actual = depIdx;

        while (ci < c.size() &&
               demandaTotal(nodos[c[ci]].productos) <= carga) {
            if (nodos[c[ci]].vehiculoAsignado != -1 && nodos[c[ci]].vehiculoAsignado != (int)vi) {
                break;
            }
            d     += dist[actual][c[ci]];
            carga -= demandaTotal(nodos[c[ci]].productos);
            actual = c[ci];
            ruta.push_back(c[ci]);
            ++ci;
        }

        d += dist[actual][depIdx];
        ruta.push_back(depIdx);
        rutasSalida.push_back(ruta);
    }

    if (ci < c.size()) d += 1e9;
    return d;
}

// Algoritmo genético VRP.
// Lógica de selección, cruce y mutación idéntica al original.
ResultadoAlgoritmo ejecutarGenetico(
    const MatrizDist& dist,
    const std::vector<Nodo>& nodos,
    const std::vector<Vehiculo>& vehiculos)
{
    auto t0 = std::chrono::high_resolution_clock::now();

    ResultadoAlgoritmo mejor;
    mejor.distanciaTotal = std::numeric_limits<double>::max();

    int depIdx = 0;
    for (int i = 0; i < static_cast<int>(nodos.size()); ++i)
        if (nodos[i].esDeposito) { depIdx = i; break; }

    std::vector<int> clienteIdxs;
    for (int i = 0; i < static_cast<int>(nodos.size()); ++i)
        if (!nodos[i].esDeposito) clienteIdxs.push_back(i);

    if (clienteIdxs.empty() || vehiculos.empty()) {
        mejor.distanciaTotal = 0.0;
        auto t1 = std::chrono::high_resolution_clock::now();
        mejor.tiempoMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        return mejor;
    }

    const int    POP    = 80;
    const int    GENS   = 300;
    const double PCROSS = 0.85;
    const double PMUT   = 0.15;

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> rReal(0.0, 1.0);
    std::uniform_int_distribution<int>     rIdx(0, static_cast<int>(clienteIdxs.size()) - 1);

    std::vector<Crom> pobla(POP, clienteIdxs);
    for (auto& cr : pobla) std::shuffle(cr.begin(), cr.end(), rng);

    for (int gen = 0; gen < GENS; ++gen) {
        std::vector<std::pair<double, int>> fit;
        for (int i = 0; i < POP; ++i) {
            std::vector<std::vector<int>> tmp;
            fit.push_back({ evaluar(pobla[i], dist, nodos, vehiculos, depIdx, tmp), i });
        }
        std::sort(fit.begin(), fit.end());

        if (fit[0].first < mejor.distanciaTotal) {
            std::vector<std::vector<int>> mr;
            mejor.distanciaTotal = evaluar(pobla[fit[0].second], dist, nodos, vehiculos, depIdx, mr);
            mejor.rutas = mr;
        }

        std::vector<Crom> nueva;
        nueva.push_back(pobla[fit[0].second]);
        nueva.push_back(pobla[fit[1].second]);

        std::uniform_int_distribution<int> rElite(0, POP / 3);

        while (static_cast<int>(nueva.size()) < POP) {
            int  p1   = fit[rElite(rng)].second;
            int  p2   = fit[rElite(rng)].second;
            Crom hijo = pobla[p1];

            if (rReal(rng) < PCROSS) {
                int a = rIdx(rng), b = rIdx(rng);
                if (a > b) std::swap(a, b);
                std::vector<bool> enHijo(nodos.size(), false);
                for (int k = a; k <= b; ++k) enHijo[hijo[k]] = true;
                int pos = (b + 1) % static_cast<int>(hijo.size());
                for (int gene : pobla[p2]) {
                    if (!enHijo[gene]) {
                        hijo[pos]    = gene;
                        enHijo[gene] = true;
                        pos = (pos + 1) % static_cast<int>(hijo.size());
                    }
                }
            }

            if (rReal(rng) < PMUT) {
                int a = rIdx(rng), b = rIdx(rng);
                std::swap(hijo[a], hijo[b]);
            }

            nueva.push_back(hijo);
        }
        pobla = nueva;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    mejor.tiempoMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return mejor;
}
