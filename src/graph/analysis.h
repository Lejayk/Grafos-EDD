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

// ordena los nodos de un grafo dirigido de forma que toda arista vaya de un
// nodo anterior a uno posterior. sirve para secuenciar tareas con dependencias.
//
// usa el algoritmo de Kahn: se empieza por los nodos sin nadie que les apunte y
// cada vez que se saca uno se descuenta a sus sucesores. si al final quedaron
// nodos sin sacar es porque se apuntan entre ellos en circulo.
//
// solo tiene sentido en grafos dirigidos: en uno no dirigido cada arista iria
// en los dos sentidos y nunca habria un orden valido.
template <typename T>
Status topologicalOrder(const GraphBase<T>& graph, DynamicArray<std::size_t>& out) {
    if (!graph.isDirected()) return NOT_DIRECTED;
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;

    // cuantas aristas entran a cada nodo. se llena a mano con ceros porque un
    // arreglo recien reservado no tiene sus elementos inicializados.
    DynamicArray<std::size_t> incoming;
    Status status = incoming.reserve(size);
    if (status != OK) return status;
    for (std::size_t i = 0; i < size; ++i) {
        status = incoming.pushBack(0);
        if (status != OK) return status;
    }

    DynamicArray<std::size_t> neighbors;
    for (std::size_t node = 0; node < size; ++node) {
        status = graph.neighborsAt(node, neighbors);
        if (status != OK) return status;
        for (std::size_t i = 0; i < neighbors.size(); ++i) {
            std::size_t target = 0;
            neighbors.get(i, target);
            std::size_t count = 0;
            incoming.get(target, count);
            incoming.set(target, count + 1);
        }
    }

    Queue<std::size_t> ready;
    for (std::size_t node = 0; node < size; ++node) {
        std::size_t count = 0;
        incoming.get(node, count);
        if (count == 0) {
            status = ready.enqueue(node);
            if (status != OK) return status;
        }
    }

    out.clear();
    while (!ready.empty()) {
        std::size_t current = 0;
        ready.dequeue(current);
        status = out.pushBack(current);
        if (status != OK) return status;

        status = graph.neighborsAt(current, neighbors);
        if (status != OK) return status;
        for (std::size_t i = 0; i < neighbors.size(); ++i) {
            std::size_t target = 0;
            neighbors.get(i, target);
            std::size_t count = 0;
            incoming.get(target, count);
            // con un grafo consistente count nunca es 0 aca, pero restarle 1 a
            // un size_t en 0 daria la vuelta a un numero enorme, asi que se
            // corta antes en lugar de confiar.
            if (count == 0) continue;
            --count;
            incoming.set(target, count);
            if (count == 0) {
                status = ready.enqueue(target);
                if (status != OK) return status;
            }
        }
    }

    if (out.size() != size) {
        // quedaron nodos atrapados en un ciclo: no hay orden posible.
        out.clear();
        return HAS_CYCLE;
    }
    return OK;
}

// tiene el grafo algun ciclo?
//
// dirigido: se intenta ordenar topologicamente. si no se puede, hay ciclo.
//
// no dirigido: se usa una propiedad de los bosques. un grafo sin ciclos con n
// nodos y c componentes tiene exactamente n - c aristas, porque cada componente
// es un arbol. cualquier arista de mas cierra un ciclo. un self-loop tambien
// cuenta, porque suma una arista sin unir componentes distintos.
template <typename T>
Status hasCycle(const GraphBase<T>& graph, bool& out) {
    std::size_t size = graph.nodeCount();
    if (size == 0) return EMPTY_GRAPH;

    if (graph.isDirected()) {
        DynamicArray<std::size_t> order;
        Status status = topologicalOrder(graph, order);
        if (status == HAS_CYCLE) {
            out = true;
            return OK;
        }
        if (status != OK) return status;
        out = false;
        return OK;
    }

    std::size_t components = 0;
    Status status = connectedComponents(graph, components);
    if (status != OK) return status;
    out = graph.edgeCount() > size - components;
    return OK;
}

}

#endif
