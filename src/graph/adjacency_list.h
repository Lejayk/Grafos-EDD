#ifndef GRAPH_ADJACENCY_LIST_H
#define GRAPH_ADJACENCY_LIST_H

#include <cstddef>
#include <new>
#include "status.h"
#include "dynamic_array.h"
#include "graph_base.h"

namespace graph {

// grafo implementado con listas de adyacencia.
//
// cada nodo tiene su propia lista enlazada de aristas, y esas listas guardan
// solo las aristas que existen de verdad. por eso ocupa memoria proporcional a
// la cantidad de aristas y no al cuadrado de la cantidad de nodos, que es la
// ventaja frente a la matriz. a cambio, preguntar si una arista existe cuesta
// recorrer la lista del nodo en lugar de mirar una sola celda.
//
// las aristas de cada lista se mantienen ordenadas por indice de destino. no es
// por prolijidad: hace que neighborsAt devuelva los vecinos en el mismo orden
// que la matriz, y asi las dos implementaciones son intercambiables de verdad.
template <typename T>
class AdjacencyList : public GraphBase<T> {
public:
    explicit AdjacencyList(bool directed = false)
        : GraphBase<T>(directed), errorState(OK) {}

    // esta clase guarda punteros crudos, asi que la regla de tres va a mano.
    // sin esto, copiar el grafo dejaria dos objetos apuntando a las mismas
    // aristas y el segundo destructor liberaria memoria ya liberada.
    AdjacencyList(const AdjacencyList& other)
        : GraphBase<T>(other.isDirected()), errorState(OK) {
        errorState = copyFrom(other);
    }

    AdjacencyList& operator=(const AdjacencyList& other) {
        if (this != &other) {
            release();
            this->directedGraph = other.isDirected();
            errorState = copyFrom(other);
        }
        return *this;
    }

    ~AdjacencyList() { release(); }

    // agrega un nodo con su lista de aristas vacia.
    Status addNode(const T& value) {
        if (indexOf(value) != NO_INDEX) return NODE_ALREADY_EXISTS;
        // se reserva lugar en los dos arreglos antes de tocar cualquiera de los
        // dos: si faltara memoria a mitad de camino quedaria un nodo sin su
        // lista de aristas, y todos los indices se desalinearian.
        Status status = values.reserve(values.size() + 1);
        if (status != OK) return status;
        status = heads.reserve(heads.size() + 1);
        if (status != OK) return status;
        values.pushBack(value);
        heads.pushBack(0);
        return OK;
    }

    std::size_t nodeCount() const { return values.size(); }

    // devuelve la posicion del nodo o NO_INDEX si no esta.
    std::size_t indexOf(const T& value) const {
        T current;
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (values.get(i, current) == OK && current == value) return i;
        }
        return NO_INDEX;
    }

    Status valueAt(std::size_t index, T& out) const {
        return values.get(index, out);
    }

    void clear() { release(); }

    // informa si alguna copia se quedo sin memoria, ya que un constructor de
    // copia no puede devolver Status.
    Status lastError() const {
        if (errorState != OK) return errorState;
        if (values.lastError() != OK) return values.lastError();
        return heads.lastError();
    }

private:
    // una arista guarda a donde va y cuanto pesa. el peso vive en la arista y
    // no hace falta marca de existencia: si el nodo esta en la lista, la arista
    // existe, aunque su peso sea cero.
    struct EdgeNode {
        std::size_t target;
        double weight;
        EdgeNode* next;
        EdgeNode(std::size_t target, double weight)
            : target(target), weight(weight), next(0) {}
    };

    DynamicArray<T> values;
    DynamicArray<EdgeNode*> heads;
    Status errorState;

    bool inRange(std::size_t index) const { return index < values.size(); }

    EdgeNode* headAt(std::size_t index) const {
        EdgeNode* head = 0;
        heads.get(index, head);
        return head;
    }

    static void releaseChain(EdgeNode* node) {
        while (node != 0) {
            EdgeNode* next = node->next;
            delete node;
            node = next;
        }
    }

    void release() {
        for (std::size_t i = 0; i < heads.size(); ++i) {
            releaseChain(headAt(i));
        }
        heads.clear();
        values.clear();
        errorState = OK;
    }

    // copia profunda: se duplica cada lista respetando el orden.
    Status copyFrom(const AdjacencyList& other) {
        Status status = values.reserve(other.values.size());
        if (status != OK) return status;
        status = heads.reserve(other.heads.size());
        if (status != OK) return status;

        for (std::size_t i = 0; i < other.values.size(); ++i) {
            T value;
            other.values.get(i, value);
            values.pushBack(value);
            heads.pushBack(0);
        }

        for (std::size_t i = 0; i < other.heads.size(); ++i) {
            EdgeNode* head = 0;
            EdgeNode* tail = 0;
            for (EdgeNode* source = other.headAt(i); source != 0; source = source->next) {
                EdgeNode* copy = new (std::nothrow) EdgeNode(source->target, source->weight);
                if (copy == 0) {
                    releaseChain(head);
                    release();
                    return OUT_OF_MEMORY;
                }
                if (tail == 0) {
                    head = copy;
                } else {
                    tail->next = copy;
                }
                tail = copy;
            }
            heads.set(i, head);
        }
        return OK;
    }
};

}

#endif
