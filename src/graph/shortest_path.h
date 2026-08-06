#ifndef GRAPH_SHORTEST_PATH_H
#define GRAPH_SHORTEST_PATH_H

#include <cstddef>
#include <cfloat>
#include "status.h"
#include "dynamic_array.h"
#include "graph_base.h"
#include "queue.h"
#include "min_heap.h"

namespace graph {

// distancia que representa "no se puede llegar". se usa como centinela en lugar
// de un infinito real para poder sumarle sin caer en valores no numericos.
const double UNREACHABLE = DBL_MAX;

// llena un arreglo con un valor repetido. hace falta porque un arreglo recien
// reservado no tiene sus elementos inicializados.
template <typename T>
Status fillWith(DynamicArray<T>& target, std::size_t size, const T& value) {
    target.clear();
    Status status = target.reserve(size);
    if (status != OK) return status;
    for (std::size_t i = 0; i < size; ++i) {
        status = target.pushBack(value);
        if (status != OK) return status;
    }
    return OK;
}

// invierte el contenido de un arreglo de indices.
inline void reversePath(DynamicArray<std::size_t>& path) {
    std::size_t count = path.size();
    if (count < 2) return;
    for (std::size_t i = 0; i < count / 2; ++i) {
        std::size_t head = 0;
        std::size_t tail = 0;
        path.get(i, head);
        path.get(count - 1 - i, tail);
        path.set(i, tail);
        path.set(count - 1 - i, head);
    }
}

// reconstruye el camino caminando hacia atras por el arreglo de predecesores.
inline Status rebuildPath(const DynamicArray<std::size_t>& previous,
                          std::size_t from, std::size_t to,
                          DynamicArray<std::size_t>& path) {
    path.clear();
    std::size_t current = to;
    // el limite de vueltas es la cantidad de nodos: si se pasa de ahi, el
    // arreglo de predecesores esta corrupto y hay que cortar en lugar de girar
    // para siempre.
    std::size_t guard = previous.size() + 1;
    while (guard > 0) {
        Status status = path.pushBack(current);
        if (status != OK) return status;
        if (current == from) {
            reversePath(path);
            return OK;
        }
        std::size_t step = NO_INDEX;
        previous.get(current, step);
        if (step == NO_INDEX) break;
        current = step;
        --guard;
    }
    path.clear();
    return NO_PATH;
}

// distancias minimas desde un nodo a todos los demas, con pesos.
//
// es el algoritmo de dijkstra: se saca siempre el nodo pendiente mas cercano al
// origen y se intenta mejorar la distancia de sus vecinos. funciona porque los
// pesos no pueden ser negativos, y por eso addEdge los rechaza en la puerta de
// entrada: con un peso negativo este algoritmo daria un resultado incorrecto sin
// avisar.
//
// deja en distances la distancia a cada nodo (UNREACHABLE si no se llega) y en
// previous el nodo anterior en el mejor camino (NO_INDEX si no tiene).
template <typename T>
Status dijkstraAt(const GraphBase<T>& graph, std::size_t from,
                  DynamicArray<double>& distances,
                  DynamicArray<std::size_t>& previous) {
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;
    if (from >= size) return INDEX_OUT_OF_RANGE;

    Status status = fillWith(distances, size, UNREACHABLE);
    if (status != OK) return status;
    status = fillWith(previous, size, NO_INDEX);
    if (status != OK) return status;
    distances.set(from, 0.0);

    MinHeap<std::size_t> pending;
    status = pending.push(0.0, from);
    if (status != OK) return status;

    DynamicArray<std::size_t> neighbors;
    while (!pending.empty()) {
        double reached = 0.0;
        std::size_t current = 0;
        pending.pop(reached, current);

        // el monticulo no permite bajar la prioridad de una entrada ya puesta,
        // asi que un nodo puede aparecer varias veces. si esta copia trae una
        // distancia peor que la ya conocida, es basura vieja: se descarta.
        double best = UNREACHABLE;
        distances.get(current, best);
        if (reached > best) continue;

        status = graph.neighborsAt(current, neighbors);
        if (status != OK) return status;

        for (std::size_t i = 0; i < neighbors.size(); ++i) {
            std::size_t next = 0;
            neighbors.get(i, next);
            double weight = 0.0;
            if (graph.weightAt(current, next, weight) != OK) continue;

            // si sumar el peso pasaria el limite del tipo, ese camino se trata
            // como inalcanzable en lugar de producir un numero sin sentido.
            if (weight > UNREACHABLE - reached) continue;
            double candidate = reached + weight;

            double known = UNREACHABLE;
            distances.get(next, known);
            if (candidate < known) {
                distances.set(next, candidate);
                previous.set(next, current);
                status = pending.push(candidate, next);
                if (status != OK) return status;
            }
        }
    }
    return OK;
}

// camino mas corto entre dos nodos, con pesos.
// deja en path los indices del recorrido, del origen al destino, y en distance
// el costo total. devuelve NO_PATH si no se puede llegar.
template <typename T>
Status shortestPathAt(const GraphBase<T>& graph, std::size_t from, std::size_t to,
                      DynamicArray<std::size_t>& path, double& distance) {
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;
    if (from >= size || to >= size) return INDEX_OUT_OF_RANGE;

    DynamicArray<double> distances;
    DynamicArray<std::size_t> previous;
    Status status = dijkstraAt(graph, from, distances, previous);
    if (status != OK) return status;

    double total = UNREACHABLE;
    distances.get(to, total);
    if (total == UNREACHABLE) {
        path.clear();
        return NO_PATH;
    }
    distance = total;
    return rebuildPath(previous, from, to, path);
}

// camino mas corto ignorando los pesos: el que pasa por menos aristas.
//
// para esto no hace falta dijkstra. un recorrido en anchura visita los nodos en
// orden de cantidad de saltos, asi que el primer momento en que se llega a un
// nodo ya es por el camino mas corto en saltos.
template <typename T>
Status shortestPathUnweightedAt(const GraphBase<T>& graph, std::size_t from, std::size_t to,
                                DynamicArray<std::size_t>& path, std::size_t& hops) {
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;
    if (from >= size || to >= size) return INDEX_OUT_OF_RANGE;

    DynamicArray<std::size_t> previous;
    Status status = fillWith(previous, size, NO_INDEX);
    if (status != OK) return status;
    DynamicArray<bool> visited;
    status = fillWith(visited, size, false);
    if (status != OK) return status;

    // aca alcanza una cola simple: no hay prioridades que ordenar porque todas
    // las aristas cuestan lo mismo. usar el monticulo seria pagar un log n por
    // nodo sin ganar nada.
    Queue<std::size_t> pending;
    status = pending.enqueue(from);
    if (status != OK) return status;
    visited.set(from, true);

    DynamicArray<std::size_t> neighbors;
    while (!pending.empty()) {
        std::size_t current = 0;
        pending.dequeue(current);

        if (current == to) {
            status = rebuildPath(previous, from, to, path);
            if (status != OK) return status;
            // la cantidad de saltos es la cantidad de aristas del camino, que
            // es un nodo menos que los nodos que lo forman.
            hops = path.size() - 1;
            return OK;
        }

        status = graph.neighborsAt(current, neighbors);
        if (status != OK) return status;

        for (std::size_t i = 0; i < neighbors.size(); ++i) {
            std::size_t next = 0;
            neighbors.get(i, next);
            bool seen = false;
            visited.get(next, seen);
            if (seen) continue;
            visited.set(next, true);
            previous.set(next, current);
            status = pending.enqueue(next);
            if (status != OK) return status;
        }
    }
    path.clear();
    return NO_PATH;
}

// versiones que reciben los valores de los nodos en lugar de sus indices.
template <typename T>
Status shortestPath(const GraphBase<T>& graph, const T& from, const T& to,
                    DynamicArray<std::size_t>& path, double& distance) {
    std::size_t origin = graph.indexOf(from);
    std::size_t target = graph.indexOf(to);
    if (origin == NO_INDEX || target == NO_INDEX) return NODE_NOT_FOUND;
    return shortestPathAt(graph, origin, target, path, distance);
}

template <typename T>
Status shortestPathUnweighted(const GraphBase<T>& graph, const T& from, const T& to,
                              DynamicArray<std::size_t>& path, std::size_t& hops) {
    std::size_t origin = graph.indexOf(from);
    std::size_t target = graph.indexOf(to);
    if (origin == NO_INDEX || target == NO_INDEX) return NODE_NOT_FOUND;
    return shortestPathUnweightedAt(graph, origin, target, path, hops);
}

template <typename T>
Status dijkstra(const GraphBase<T>& graph, const T& from,
                DynamicArray<double>& distances,
                DynamicArray<std::size_t>& previous) {
    std::size_t origin = graph.indexOf(from);
    if (origin == NO_INDEX) return NODE_NOT_FOUND;
    return dijkstraAt(graph, origin, distances, previous);
}

}

#endif
