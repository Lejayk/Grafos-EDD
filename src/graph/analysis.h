#ifndef GRAPH_ANALYSIS_H
#define GRAPH_ANALYSIS_H

#include <cstddef>
#include "status.h"
#include "dynamic_array.h"
#include "graph_base.h"
#include "queue.h"
#include "traversal.h"

namespace graph {

// consultas sobre la forma del grafo: grado, conexidad, componentes y caminos.
// igual que los recorridos, son funciones libres escritas contra GraphBase, asi
// que sirven para las dos implementaciones sin cambiar nada.

// cantidad de vecinos del nodo.
// ojo: en la teoria clasica un self-loop suma 2 al grado, pero aca la arista
// figura una sola vez en la adyacencia, asi que suma 1.
template <typename T>
Status degreeAt(const GraphBase<T>& graph, std::size_t index, std::size_t& out) {
    DynamicArray<std::size_t> neighbors;
    Status status = graph.neighborsAt(index, neighbors);
    if (status != OK) return status;
    out = neighbors.size();
    return OK;
}

template <typename T>
Status degree(const GraphBase<T>& graph, const T& value, std::size_t& out) {
    std::size_t index = graph.indexOf(value);
    if (index == NO_INDEX) return NODE_NOT_FOUND;
    return degreeAt(graph, index, out);
}

// vecinos ignorando el sentido de las aristas: los sucesores mas, si el grafo
// es dirigido, los predecesores.
//
// hace falta porque conexidad y componentes tienen que significar lo mismo en
// los dos casos: nodos unidos por una arista, sin importar hacia donde apunta.
// si en un grafo dirigido se siguieran solo las flechas, A->B y C->B darian
// tres componentes en lugar de uno.
template <typename T>
Status undirectedNeighborsAt(const GraphBase<T>& graph, std::size_t index,
                             DynamicArray<std::size_t>& out) {
    Status status = graph.neighborsAt(index, out);
    if (status != OK) return status;
    if (!graph.isDirected()) return OK;

    for (std::size_t i = 0; i < graph.nodeCount(); ++i) {
        if (i == index) continue;
        if (!graph.hasEdgeAt(i, index)) continue;
        bool already = false;
        for (std::size_t j = 0; j < out.size(); ++j) {
            std::size_t existing = 0;
            out.get(j, existing);
            if (existing == i) {
                already = true;
                break;
            }
        }
        if (!already) {
            status = out.pushBack(i);
            if (status != OK) return status;
        }
    }
    return OK;
}

// cantidad de grupos de nodos conectados entre si, ignorando el sentido de las
// aristas. un grafo conexo tiene exactamente uno.
template <typename T>
Status connectedComponents(const GraphBase<T>& graph, std::size_t& out) {
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;

    DynamicArray<bool> visited;
    Status status = buildVisited(visited, size);
    if (status != OK) return status;

    std::size_t total = 0;
    Queue<std::size_t> pending;
    DynamicArray<std::size_t> neighbors;

    for (std::size_t start = 0; start < size; ++start) {
        if (wasVisited(visited, start)) continue;
        ++total;
        visited.set(start, true);
        status = pending.enqueue(start);
        if (status != OK) return status;

        while (!pending.empty()) {
            std::size_t current = 0;
            pending.dequeue(current);
            status = undirectedNeighborsAt(graph, current, neighbors);
            if (status != OK) return status;
            for (std::size_t i = 0; i < neighbors.size(); ++i) {
                std::size_t next = 0;
                neighbors.get(i, next);
                if (!wasVisited(visited, next)) {
                    visited.set(next, true);
                    status = pending.enqueue(next);
                    if (status != OK) return status;
                }
            }
        }
    }
    out = total;
    return OK;
}

template <typename T>
Status isConnected(const GraphBase<T>& graph, bool& out) {
    std::size_t components = 0;
    Status status = connectedComponents(graph, components);
    if (status != OK) return status;
    out = components == 1;
    return OK;
}

// existe un camino de from a to?
// aca SI se respeta el sentido de las aristas: en un grafo dirigido se puede
// llegar de A a B sin que exista camino de B a A.
template <typename T>
Status hasPathAt(const GraphBase<T>& graph, std::size_t from, std::size_t to, bool& out) {
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;
    if (from >= size || to >= size) return INDEX_OUT_OF_RANGE;

    DynamicArray<std::size_t> visitOrder;
    Status status = breadthFirstAt(graph, from, visitOrder);
    if (status != OK) return status;

    for (std::size_t i = 0; i < visitOrder.size(); ++i) {
        std::size_t visited = 0;
        visitOrder.get(i, visited);
        if (visited == to) {
            out = true;
            return OK;
        }
    }
    out = false;
    return OK;
}

template <typename T>
Status hasPath(const GraphBase<T>& graph, const T& from, const T& to, bool& out) {
    std::size_t origin = graph.indexOf(from);
    std::size_t target = graph.indexOf(to);
    if (origin == NO_INDEX || target == NO_INDEX) return NODE_NOT_FOUND;
    return hasPathAt(graph, origin, target, out);
}

}

#endif
