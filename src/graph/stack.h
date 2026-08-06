#ifndef GRAPH_STACK_H
#define GRAPH_STACK_H

#include <cstddef>
#include <new>
#include "status.h"

namespace graph {

// pila lifo propia, usada por el recorrido en profundidad iterativo.
//
// se apila y se desapila siempre por la cabeza, asi que las dos operaciones
// son de tiempo constante y no hace falta guardar un puntero al final.
//
// que le pide a T: constructor por defecto, constructor de copia y operator=.
template <typename T>
class Stack {
public:
    Stack() : head(0), count(0), errorState(OK) {}

    Stack(const Stack& other) : head(0), count(0), errorState(OK) {
        errorState = copyFrom(other);
    }

    Stack& operator=(const Stack& other) {
        if (this != &other) {
            release();
            errorState = copyFrom(other);
        }
        return *this;
    }

    ~Stack() { release(); }

    std::size_t size() const { return count; }
    bool empty() const { return count == 0; }

    Status lastError() const { return errorState; }

    // pone un elemento arriba de la pila.
    Status push(const T& value) {
        Node* node = new (std::nothrow) Node(value);
        if (node == 0) return OUT_OF_MEMORY;
        node->next = head;
        head = node;
        ++count;
        return OK;
    }

    // saca el elemento de arriba y lo deja en out.
    Status pop(T& out) {
        if (head == 0) return EMPTY_CONTAINER;
        Node* node = head;
        out = node->value;
        head = node->next;
        delete node;
        --count;
        return OK;
    }

    // mira el elemento de arriba sin sacarlo.
    Status top(T& out) const {
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
    std::size_t count;
    Status errorState;

    void release() {
        while (head != 0) {
            Node* next = head->next;
            delete head;
            head = next;
        }
        count = 0;
    }

    // copiar una pila tiene una trampa: si se recorre el original de arriba
    // hacia abajo apilando sobre la copia, la copia queda al reves. por eso se
    // arma primero una cadena invertida y despues se vuelca, que la deja en el
    // orden correcto.
    Status copyFrom(const Stack& other) {
        Node* reversed = 0;
        for (Node* node = other.head; node != 0; node = node->next) {
            Node* copy = new (std::nothrow) Node(node->value);
            if (copy == 0) {
                releaseChain(reversed);
                return OUT_OF_MEMORY;
            }
            copy->next = reversed;
            reversed = copy;
        }
        while (reversed != 0) {
            Node* node = reversed;
            reversed = reversed->next;
            node->next = head;
            head = node;
            ++count;
        }
        return OK;
    }

    static void releaseChain(Node* node) {
        while (node != 0) {
            Node* next = node->next;
            delete node;
            node = next;
        }
    }
};

}

#endif
