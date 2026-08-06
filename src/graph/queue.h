#ifndef GRAPH_QUEUE_H
#define GRAPH_QUEUE_H

#include <cstddef>
#include <new>
#include "status.h"

namespace graph {

// cola fifo propia, usada por el recorrido en anchura.
//
// esta hecha con nodos enlazados y no con un arreglo: encolar y desencolar
// quedan en tiempo constante y nunca hay que reacomodar elementos.
//
// que le pide a T: constructor por defecto, constructor de copia y operator=.
template <typename T>
class Queue {
public:
    Queue() : head(0), tail(0), count(0), errorState(OK) {}

    Queue(const Queue& other) : head(0), tail(0), count(0), errorState(OK) {
        errorState = copyFrom(other);
    }

    Queue& operator=(const Queue& other) {
        if (this != &other) {
            release();
            errorState = copyFrom(other);
        }
        return *this;
    }

    ~Queue() { release(); }

    std::size_t size() const { return count; }
    bool empty() const { return count == 0; }

    // un constructor de copia no puede devolver Status, asi que si la copia se
    // queda sin memoria el error se guarda aca.
    Status lastError() const { return errorState; }

    // agrega un elemento al final de la cola.
    Status enqueue(const T& value) {
        Node* node = new (std::nothrow) Node(value);
        if (node == 0) return OUT_OF_MEMORY;
        if (tail == 0) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        ++count;
        return OK;
    }

    // saca el elemento del frente y lo deja en out.
    Status dequeue(T& out) {
        if (head == 0) return EMPTY_CONTAINER;
        Node* node = head;
        out = node->value;
        head = node->next;
        if (head == 0) tail = 0;
        delete node;
        --count;
        return OK;
    }

    // mira el elemento del frente sin sacarlo.
    Status front(T& out) const {
        if (head == 0) return EMPTY_CONTAINER;
        out = head->value;
        return OK;
    }

    void clear() {
        release();
        errorState = OK;
    }

private:
    struct Node {
        T value;
        Node* next;
        explicit Node(const T& nodeValue) : value(nodeValue), next(0) {}
    };

    Node* head;
    Node* tail;
    std::size_t count;
    Status errorState;

    void release() {
        while (head != 0) {
            Node* next = head->next;
            delete head;
            head = next;
        }
        tail = 0;
        count = 0;
    }

    // copia el contenido de other respetando el orden.
    // si se queda sin memoria a mitad de camino deja la cola vacia en lugar de
    // dejarla copiada por la mitad, que seria peor.
    Status copyFrom(const Queue& other) {
        for (Node* node = other.head; node != 0; node = node->next) {
            Status status = enqueue(node->value);
            if (status != OK) {
                release();
                return status;
            }
        }
        return OK;
    }
};

}

#endif
