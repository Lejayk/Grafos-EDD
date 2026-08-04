#ifndef GRAPH_ADJACENCY_MATRIX_H
#define GRAPH_ADJACENCY_MATRIX_H

#include <cstddef>
#include "status.h"
#include "dynamic_array.h"
#include "graph_base.h"

namespace graph {

// grafo implementado con matriz de adyacencia.
//
// la matriz se guarda plana: un solo arreglo de n*n celdas donde la celda de
// la fila i y la columna j vive en la posicion i*n+j. es memoria contigua y
// evita tener que anidar un arreglo dinamico dentro de otro.
//
// consultar si existe una arista es de tiempo constante, que es la ventaja de
// esta representacion frente a la de listas. a cambio ocupa n*n celdas aunque
// el grafo tenga pocas aristas.
template <typename T>
class AdjacencyMatrix : public GraphBase<T> {
public:
    explicit AdjacencyMatrix(bool directed = false) : GraphBase<T>(directed) {}

    // agrega un nodo al final y hace crecer la matriz a (n+1)x(n+1).
    // como la posicion de una celda depende del ancho de la fila, no alcanza
    // con agregar celdas al final: hay que reconstruir la matriz completa.
    Status addNode(const T& value) {
        if (indexOf(value) != NO_INDEX) return NODE_ALREADY_EXISTS;

        std::size_t oldSize = values.size();
        std::size_t newSize = oldSize + 1;

        DynamicArray<Cell> rebuilt;
        Status status = rebuilt.reserve(newSize * newSize);
        if (status != OK) return status;

        Cell blank;
        for (std::size_t row = 0; row < newSize; ++row) {
            for (std::size_t column = 0; column < newSize; ++column) {
                if (row < oldSize && column < oldSize) {
                    Cell existing;
                    cells.get(row * oldSize + column, existing);
                    rebuilt.pushBack(existing);
                } else {
                    rebuilt.pushBack(blank);
                }
            }
        }

        // el arreglo nuevo ya esta completo, asi que recien ahora se toca el
        // estado del grafo. si pushBack falla no se modifico nada.
        status = values.pushBack(value);
        if (status != OK) return status;

        cells.swap(rebuilt);
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

    void clear() {
        values.clear();
        cells.clear();
    }

    // informa si alguna copia se quedo sin memoria, ya que un constructor de
    // copia no puede devolver Status.
    Status lastError() const {
        if (values.lastError() != OK) return values.lastError();
        return cells.lastError();
    }

private:
    // una arista de peso cero es una arista de verdad, asi que la existencia
    // no se puede deducir del peso: va en un campo aparte.
    struct Cell {
        bool present;
        double weight;
        Cell() : present(false), weight(0.0) {}
    };

    DynamicArray<T> values;
    DynamicArray<Cell> cells;

    bool inRange(std::size_t index) const { return index < values.size(); }

    Cell readCell(std::size_t row, std::size_t column) const {
        Cell cell;
        cells.get(row * values.size() + column, cell);
        return cell;
    }

    void writeCell(std::size_t row, std::size_t column, bool present, double weight) {
        Cell cell;
        cell.present = present;
        cell.weight = weight;
        cells.set(row * values.size() + column, cell);
    }
};

}

#endif
