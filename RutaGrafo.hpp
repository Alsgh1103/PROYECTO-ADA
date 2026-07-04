#pragma once
#include "Estructuras.hpp"
#include <algorithm>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  Dijkstra con reconstrucción de camino
//  Retorna distancia en metros. camino = secuencia de NodoGrafo::id.
// ─────────────────────────────────────────────────────────────────────────────
inline float dijkstra(const Grafo& g, int origen, int destino,
                      std::vector<int>& camino) {
    const float INF = std::numeric_limits<float>::max();
    int N = static_cast<int>(g.nodos.size());
    std::vector<float> dist(N, INF);
    std::vector<int>   prev(N, -1);

    using pfi = std::pair<float, int>;
    std::priority_queue<pfi, std::vector<pfi>, std::greater<pfi>> pq;
    dist[origen] = 0.f;
    pq.push({0.f, origen});

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u] + 1e-6f) continue;
        if (u == destino) break;
        for (const auto& [v, w] : g.adj[u]) {
            float nd = dist[u] + w;
            if (nd < dist[v]) {
                dist[v] = nd;
                prev[v] = u;
                pq.push({nd, v});
            }
        }
    }

    // Reconstruir camino
    camino.clear();
    if (dist[destino] >= INF) {
        // Sin camino: retornar el origen y destino directamente como fallback
        camino = {origen, destino};
        return INF;
    }
    for (int cur = destino; cur != -1; cur = prev[cur])
        camino.push_back(cur);
    std::reverse(camino.begin(), camino.end());
    return dist[destino];
}

// ─────────────────────────────────────────────────────────────────────────────
//  Precalcula la matriz de distancias (metros) y caminos entre todos los
//  nodos VRP (depósito + tiendas). Se llama UNA VEZ antes de los algoritmos.
//  Los algoritmos reciben la MatrizDist y usan dist[i][j] en lugar de distM().
// ─────────────────────────────────────────────────────────────────────────────
inline InfoRutas precalcularRutas(const Grafo& g,
                                   const std::vector<Nodo>& nodos) {
    int n = static_cast<int>(nodos.size());
    InfoRutas info;
    info.distancias.assign(n, std::vector<float>(n, 0.f));
    info.caminos.resize(n, std::vector<std::vector<int>>(n));

    for (int i = 0; i < n; ++i) {
        info.caminos[i][i] = {nodos[i].grafoNodeId};
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            info.distancias[i][j] = dijkstra(
                g,
                nodos[i].grafoNodeId,
                nodos[j].grafoNodeId,
                info.caminos[i][j]
            );
        }
    }
    return info;
}
