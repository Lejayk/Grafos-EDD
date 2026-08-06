#ifndef GRAPH_DYNAMIC_ARRAY_H
#define GRAPH_DYNAMIC_ARRAY_H

#include <cstddef>
#include <new>
#include "status.h"

namespace graph {

// arreglo dinamico propio, usado en toda la libreria en lugar de std::vector.
//
// no expone operator[] a proposito: todo acceso pasa por get/set, que validan
// el indice y devuelven un Status. asi es imposible leer o escribir fuera de
// rango sin enterarse.
//
// que le pide a T: constructor por defecto, constructor de copia y operator=.
template <typename T>
class DynamicArray {
public:
    DynamicArray() : items(0), count(0), capacityValue(0), errorState(OK) {}

    explicit DynamicArray(std::size_t initialCapacity)
        : items(0), count(0), capacityValue(0), errorState(OK) {
        errorState = reserve(initialCapacity);
    }

    DynamicArray(const DynamicArray& other)
        : items(0), count(0), capacityValue(0), errorState(OK) {
        errorState = copyFrom(other);
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            release();
            errorState = copyFrom(other);
        }
        return *this;
    }

    ~DynamicArray() { release(); }

    std::size_t size() const { return count; }
    std::size_t capacity() const { return capacityValue; }
    bool empty() const { return count == 0; }

    // guarda el ultimo fallo de las operaciones que no pueden devolver Status
    // por si mismas: el constructor de copia, el operator= y el constructor
    // que recibe una capacidad inicial. revisalo despues de cualquiera de esos
    // tres. las demas operaciones devuelven su Status directamente.
    Status lastError() const { return errorState; }

    // reserva espacio para al menos newCapacity elementos.
    // nunca reduce la capacidad ya reservada.
    Status reserve(std::size_t newCapacity) {
        if (newCapacity <= capacityValue) return OK;
        if (newCapacity > maxCapacity()) return CAPACITY_OVERFLOW;
        T* buffer = new (std::nothrow) T[newCapacity];
        if (buffer == 0) return OUT_OF_MEMORY;
        for (std::size_t i = 0; i < count; ++i) {
            buffer[i] = items[i];
        }
        delete[] items;
        items = buffer;
        capacityValue = newCapacity;
        return OK;
    }

    Status pushBack(const T& value) {
        if (count == capacityValue) {
            Status status = grow();
            if (status != OK) return status;
        }
        items[count] = value;
        ++count;
        return OK;
    }

    Status get(std::size_t index, T& out) const {
        if (index >= count) return INDEX_OUT_OF_RANGE;
        out = items[index];
        return OK;
    }

    Status set(std::size_t index, const T& value) {
        if (index >= count) return INDEX_OUT_OF_RANGE;
        items[index] = value;
        return OK;
    }

    // saca el elemento del indice y corre los siguientes una posicion.
    Status removeAt(std::size_t index) {
        if (index >= count) return INDEX_OUT_OF_RANGE;
        for (std::size_t i = index; i + 1 < count; ++i) {
            items[i] = items[i + 1];
        }
        --count;
        return OK;
    }

    // deja el arreglo vacio pero conserva la memoria ya reservada,
    // para no pagar el costo de volver a pedirla.
    void clear() {
        count = 0;
        errorState = OK;
    }

    // intercambia el contenido de dos arreglos moviendo punteros, sin copiar
    // ni pedir memoria. sirve para reemplazar de golpe el contenido de un
    // arreglo por otro ya construido: no puede fallar y no deja al objeto en
    // un estado intermedio si el sistema se queda sin memoria.
    void swap(DynamicArray& other) noexcept {
        T* otherItems = other.items;
        other.items = items;
        items = otherItems;

        std::size_t otherCount = other.count;
        other.count = count;
        count = otherCount;

        std::size_t otherCapacity = other.capacityValue;
        other.capacityValue = capacityValue;
        capacityValue = otherCapacity;

        Status otherError = other.errorState;
        other.errorState = errorState;
        errorState = otherError;
    }

private:
    T* items;
    std::size_t count;
    std::size_t capacityValue;
    Status errorState;

    // enum en vez de static const para que sea siempre una constante de
    // compilacion, sin necesidad de definirla fuera de la clase.
    enum { INITIAL_CAPACITY = 4 };

    // cuantos elementos T caben como maximo antes de que el calculo interno de
    // bytes que hace new[] desborde un size_t.
    static std::size_t maxCapacity() {
        return static_cast<std::size_t>(-1) / sizeof(T);
    }

    // duplica la capacidad cuidando que la multiplicacion no de la vuelta.
    Status grow() {
        if (capacityValue == 0) return reserve(INITIAL_CAPACITY);
        if (capacityValue > maxCapacity() / 2) return CAPACITY_OVERFLOW;
        return reserve(capacityValue * 2);
    }

    void release() {
        delete[] items;
        items = 0;
        count = 0;
        capacityValue = 0;
    }

    Status copyFrom(const DynamicArray& other) {
        if (other.count == 0) return OK;
        T* buffer = new (std::nothrow) T[other.count];
        if (buffer == 0) return OUT_OF_MEMORY;
        for (std::size_t i = 0; i < other.count; ++i) {
            buffer[i] = other.items[i];
        }
        items = buffer;
        count = other.count;
        capacityValue = other.count;
        return OK;
    }
};

}

#endif
