#ifndef GRAPH_MIN_HEAP_H
#define GRAPH_MIN_HEAP_H

#include <cstddef>
#include "status.h"
#include "dynamic_array.h"

namespace graph {

// monticulo minimo: cola de prioridad donde siempre sale primero el elemento de
// menor prioridad. la usa dijkstra para elegir en cada paso el nodo mas cercano
// sin tener que recorrer todos.
//
// la prioridad es un double aparte del valor, no se le pide a T que sepa
// compararse. asi T puede ser cualquier cosa.
//
// esta armado sobre el arreglo dinamico: el hijo izquierdo del nodo i vive en
// 2i+1 y el derecho en 2i+2. por eso un arbol completo no necesita punteros.
template <typename T>
class MinHeap {
public:
    MinHeap() {}

    std::size_t size() const { return items.size(); }
    bool empty() const { return items.empty(); }
    Status lastError() const { return items.lastError(); }

    void clear() { items.clear(); }

    Status push(double priority, const T& value) {
        Entry entry;
        entry.priority = priority;
        entry.value = value;
        Status status = items.pushBack(entry);
        if (status != OK) return status;
        siftUp(items.size() - 1);
        return OK;
    }

    // saca el elemento de menor prioridad.
    Status pop(double& priority, T& out) {
        if (items.empty()) return EMPTY_CONTAINER;
        Entry root;
        items.get(0, root);
        priority = root.priority;
        out = root.value;

        // el ultimo pasa a la raiz y baja hasta su lugar.
        std::size_t last = items.size() - 1;
        if (last > 0) {
            Entry lastEntry;
            items.get(last, lastEntry);
            items.set(0, lastEntry);
        }
        items.removeAt(last);
        siftDown(0);
        return OK;
    }

    Status pop(T& out) {
        double ignored = 0.0;
        return pop(ignored, out);
    }

    // mira el proximo sin sacarlo.
    Status top(double& priority, T& out) const {
        if (items.empty()) return EMPTY_CONTAINER;
        Entry root;
        items.get(0, root);
        priority = root.priority;
        out = root.value;
        return OK;
    }

private:
    struct Entry {
        double priority;
        T value;
        Entry() : priority(0.0), value() {}
    };

    DynamicArray<Entry> items;

    // sube el elemento mientras sea menor que su padre.
    void siftUp(std::size_t index) {
        while (index > 0) {
            std::size_t parent = (index - 1) / 2;
            Entry child;
            Entry parentEntry;
            items.get(index, child);
            items.get(parent, parentEntry);
            if (!(child.priority < parentEntry.priority)) break;
            items.set(index, parentEntry);
            items.set(parent, child);
            index = parent;
        }
    }

    // baja el elemento mientras alguno de sus hijos sea menor.
    void siftDown(std::size_t index) {
        std::size_t size = items.size();
        // en un monticulo de base cero los nodos internos son exactamente los
        // indices menores a size/2. usar ese limite en lugar de comprobar
        // 2i+1 < size evita que la multiplicacion pueda desbordar.
        std::size_t internalNodes = size / 2;
        while (index < internalNodes) {
            std::size_t left = index * 2 + 1;
            std::size_t right = left + 1;
            std::size_t smallest = left;
            if (right < size) {
                Entry leftEntry;
                Entry rightEntry;
                items.get(left, leftEntry);
                items.get(right, rightEntry);
                if (rightEntry.priority < leftEntry.priority) smallest = right;
            }
            Entry current;
            Entry best;
            items.get(index, current);
            items.get(smallest, best);
            if (!(best.priority < current.priority)) break;
            items.set(index, best);
            items.set(smallest, current);
            index = smallest;
        }
    }
};

}

#endif
