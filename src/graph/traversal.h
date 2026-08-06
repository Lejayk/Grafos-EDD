#ifndef GRAPH_TRAVERSAL_H
#define GRAPH_TRAVERSAL_H

#include <cstddef>
#include "status.h"
#include "dynamic_array.h"
#include "graph_base.h"
#include "queue.h"
#include "stack.h"

namespace graph {

// recorridos del grafo.
//
// son funciones libres y no metodos: reciben una referencia a GraphBase, asi
// que estan escritos UNA sola vez y corren igual sobre listas que sobre matriz.
// lo unico que le piden al grafo es neighborsAt.

// marca de visitados. se llena a mano con false porque un arreglo recien
// reservado no tiene sus elementos inicializados.
inline Status buildVisited(DynamicArray<bool>& visited, std::size_t size) {
    visited.clear();
    Status status = visited.reserve(size);
    if (status != OK) return status;
    for (std::size_t i = 0; i < size; ++i) {
        status = visited.pushBack(false);
        if (status != OK) return status;
    }
    return OK;
}

inline bool wasVisited(const DynamicArray<bool>& visited, std::size_t index) {
    bool seen = false;
    visited.get(index, seen);
    return seen;
}

// recorrido en anchura desde un nodo: visita primero todos los vecinos
// directos, despues los vecinos de esos, y asi. deja en out los indices en
// orden de visita.
template <typename T>
Status breadthFirstAt(const GraphBase<T>& graph, std::size_t start,
                      DynamicArray<std::size_t>& out) {
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;
    if (start >= size) return INDEX_OUT_OF_RANGE;

    out.clear();
    DynamicArray<bool> visited;
    Status status = buildVisited(visited, size);
    if (status != OK) return status;

    Queue<std::size_t> pending;
    status = pending.enqueue(start);
    if (status != OK) return status;
    visited.set(start, true);

    DynamicArray<std::size_t> neighbors;
    while (!pending.empty()) {
        std::size_t current = 0;
        pending.dequeue(current);
        status = out.pushBack(current);
        if (status != OK) return status;

        status = graph.neighborsAt(current, neighbors);
        if (status != OK) return status;

        for (std::size_t i = 0; i < neighbors.size(); ++i) {
            std::size_t next = 0;
            neighbors.get(i, next);
            if (!wasVisited(visited, next)) {
                // se marca al encolar, no al sacar: si no, un nodo con dos
                // vecinos en comun entraria dos veces a la cola.
                visited.set(next, true);
                status = pending.enqueue(next);
                if (status != OK) return status;
            }
        }
    }
    return OK;
}

// recorrido en profundidad desde un nodo: agota una rama antes de pasar a la
// siguiente. es iterativo, con pila propia, para no depender de la pila de
// llamadas del programa y poder recorrer grafos grandes sin desbordarla.
template <typename T>
Status depthFirstAt(const GraphBase<T>& graph, std::size_t start,
                    DynamicArray<std::size_t>& out) {
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;
    if (start >= size) return INDEX_OUT_OF_RANGE;

    out.clear();
    DynamicArray<bool> visited;
    Status status = buildVisited(visited, size);
    if (status != OK) return status;

    Stack<std::size_t> pending;
    status = pending.push(start);
    if (status != OK) return status;

    DynamicArray<std::size_t> neighbors;
    while (!pending.empty()) {
        std::size_t current = 0;
        pending.pop(current);
        // aca se marca al SACAR y no al apilar: un nodo puede estar apilado
        // varias veces por distintos caminos, y gana el primero que sale.
        if (wasVisited(visited, current)) continue;
        visited.set(current, true);

        status = out.pushBack(current);
        if (status != OK) return status;

        status = graph.neighborsAt(current, neighbors);
        if (status != OK) return status;

        // los vecinos se apilan al REVES para que salgan en orden ascendente,
        // igual que los visitaria una version recursiva.
        for (std::size_t i = neighbors.size(); i > 0; --i) {
            std::size_t next = 0;
            neighbors.get(i - 1, next);
            if (!wasVisited(visited, next)) {
                status = pending.push(next);
                if (status != OK) return status;
            }
        }
    }
    return OK;
}

// versiones que reciben el valor del nodo en lugar de su indice.
template <typename T>
Status breadthFirst(const GraphBase<T>& graph, const T& start,
                    DynamicArray<std::size_t>& out) {
    std::size_t index = graph.indexOf(start);
    if (index == NO_INDEX) return NODE_NOT_FOUND;
    return breadthFirstAt(graph, index, out);
}

template <typename T>
Status depthFirst(const GraphBase<T>& graph, const T& start,
                  DynamicArray<std::size_t>& out) {
    std::size_t index = graph.indexOf(start);
    if (index == NO_INDEX) return NODE_NOT_FOUND;
    return depthFirstAt(graph, index, out);
}

}

#endif
