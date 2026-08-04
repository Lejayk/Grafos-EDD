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

    // saca un nodo del grafo junto con todas sus aristas.
    // hay dos cosas que arreglar: las aristas que APUNTABAN al nodo borrado
    // quedan colgadas y hay que sacarlas de las listas de los demas, y los
    // indices mayores al borrado se corren un lugar, asi que las aristas que
    // los referencian tienen que corregir su destino.
    Status removeNode(const T& value) {
        std::size_t target = indexOf(value);
        if (target == NO_INDEX) return NODE_NOT_FOUND;

        releaseChain(headAt(target));
        heads.removeAt(target);
        values.removeAt(target);

        for (std::size_t i = 0; i < heads.size(); ++i) {
            EdgeNode* head = headAt(i);
            EdgeNode* previous = 0;
            EdgeNode* current = head;
            while (current != 0) {
                EdgeNode* next = current->next;
                if (current->target == target) {
                    if (previous == 0) {
                        head = next;
                    } else {
                        previous->next = next;
                    }
                    delete current;
                } else {
                    if (current->target > target) --current->target;
                    previous = current;
                }
                current = next;
            }
            heads.set(i, head);
        }
        return OK;
    }

    // agrega una arista entre dos nodos identificados por su valor.
    // si ya existia, actualiza el peso en lugar de duplicarla.
    Status addEdge(const T& from, const T& to, double weight = 1.0) {
        std::size_t origin = indexOf(from);
        std::size_t target = indexOf(to);
        if (origin == NO_INDEX || target == NO_INDEX) return NODE_NOT_FOUND;
        return addEdgeAt(origin, target, weight);
    }

    Status removeEdge(const T& from, const T& to) {
        std::size_t origin = indexOf(from);
        std::size_t target = indexOf(to);
        if (origin == NO_INDEX || target == NO_INDEX) return NODE_NOT_FOUND;
        return removeEdgeAt(origin, target);
    }

    // en un grafo no dirigido la arista se guarda en las dos listas, salvo que
    // sea un self-loop: ahi las dos direcciones son la misma lista y el mismo
    // destino, asi que escribirla dos veces la duplicaria.
    Status addEdgeAt(std::size_t from, std::size_t to, double weight = 1.0) {
        if (!inRange(from) || !inRange(to)) return INDEX_OUT_OF_RANGE;
        Status status = GraphBase<T>::validateWeight(weight);
        if (status != OK) return status;

        bool mirrored = !this->isDirected() && from != to;

        // se guarda el estado anterior para poder dejar todo como estaba si la
        // segunda direccion no consigue memoria. una arista a medias en un
        // grafo no dirigido es peor que no haberla agregado.
        double previousWeight = 0.0;
        const EdgeNode* existing = findEdge(from, to);
        bool existed = existing != 0;
        if (existed) previousWeight = existing->weight;

        status = setEdge(from, to, weight);
        if (status != OK) return status;

        if (mirrored) {
            status = setEdge(to, from, weight);
            if (status != OK) {
                if (existed) setEdge(from, to, previousWeight);
                else unsetEdge(from, to);
                return status;
            }
        }
        return OK;
    }

    Status removeEdgeAt(std::size_t from, std::size_t to) {
        if (!inRange(from) || !inRange(to)) return INDEX_OUT_OF_RANGE;
        if (findEdge(from, to) == 0) return EDGE_NOT_FOUND;
        unsetEdge(from, to);
        if (!this->isDirected() && from != to) unsetEdge(to, from);
        return OK;
    }

    std::size_t nodeCount() const { return values.size(); }

    // en un grafo no dirigido cada arista aparece en dos listas, asi que
    // contarlas todas daria el doble. contando solo las que van hacia un indice
    // mayor o igual al propio, cada arista se cuenta una vez y los self-loops
    // tambien quedan contados una sola vez.
    std::size_t edgeCount() const {
        std::size_t total = 0;
        for (std::size_t i = 0; i < heads.size(); ++i) {
            for (const EdgeNode* current = headAt(i); current != 0; current = current->next) {
                if (!this->isDirected() && current->target < i) continue;
                ++total;
            }
        }
        return total;
    }

    bool hasEdgeAt(std::size_t from, std::size_t to) const {
        if (!inRange(from) || !inRange(to)) return false;
        return findEdge(from, to) != 0;
    }

    Status weightAt(std::size_t from, std::size_t to, double& out) const {
        if (!inRange(from) || !inRange(to)) return INDEX_OUT_OF_RANGE;
        const EdgeNode* edge = findEdge(from, to);
        if (edge == 0) return EDGE_NOT_FOUND;
        out = edge->weight;
        return OK;
    }

    // recorre la lista del nodo y devuelve los indices de sus vecinos.
    // como la lista esta ordenada por destino, salen en orden ascendente, igual
    // que en la matriz.
    Status neighborsAt(std::size_t index, DynamicArray<std::size_t>& out) const {
        if (!inRange(index)) return INDEX_OUT_OF_RANGE;
        out.clear();
        for (const EdgeNode* current = headAt(index); current != 0; current = current->next) {
            Status status = out.pushBack(current->target);
            if (status != OK) return status;
        }
        return OK;
    }

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
        EdgeNode(std::size_t targetIndex, double edgeWeight)
            : target(targetIndex), weight(edgeWeight), next(0) {}
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

    // busca una arista aprovechando que la lista esta ordenada por destino:
    // en cuanto se pasa del indice buscado ya se puede cortar.
    const EdgeNode* findEdge(std::size_t from, std::size_t to) const {
        for (const EdgeNode* current = headAt(from); current != 0; current = current->next) {
            if (current->target == to) return current;
            if (current->target > to) break;
        }
        return 0;
    }

    // inserta la arista manteniendo el orden por destino, o actualiza el peso si
    // ya estaba.
    Status setEdge(std::size_t from, std::size_t to, double weight) {
        EdgeNode* head = headAt(from);
        EdgeNode* previous = 0;
        EdgeNode* current = head;
        while (current != 0 && current->target < to) {
            previous = current;
            current = current->next;
        }
        if (current != 0 && current->target == to) {
            current->weight = weight;
            return OK;
        }
        EdgeNode* node = new (std::nothrow) EdgeNode(to, weight);
        if (node == 0) return OUT_OF_MEMORY;
        node->next = current;
        if (previous == 0) {
            heads.set(from, node);
        } else {
            previous->next = node;
        }
        return OK;
    }

    // desengancha la arista y libera su nodo. devuelve si la encontro.
    bool unsetEdge(std::size_t from, std::size_t to) {
        EdgeNode* previous = 0;
        EdgeNode* current = headAt(from);
        while (current != 0) {
            if (current->target == to) {
                if (previous == 0) {
                    heads.set(from, current->next);
                } else {
                    previous->next = current->next;
                }
                delete current;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
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
