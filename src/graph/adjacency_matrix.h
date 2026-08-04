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

    // agrega una arista entre dos nodos identificados por su valor.
    // si la arista ya existia, actualiza el peso en lugar de duplicarla: una
    // matriz de adyacencia no puede representar dos aristas entre el mismo par.
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

    // version por indice, para quien ya resolvio los nodos.
    // en un grafo no dirigido la arista se escribe en las dos celdas.
    Status addEdgeAt(std::size_t from, std::size_t to, double weight = 1.0) {
        if (!inRange(from) || !inRange(to)) return INDEX_OUT_OF_RANGE;
        Status status = GraphBase<T>::validateWeight(weight);
        if (status != OK) return status;
        writeCell(from, to, true, weight);
        if (!this->isDirected()) writeCell(to, from, true, weight);
        return OK;
    }

    Status removeEdgeAt(std::size_t from, std::size_t to) {
        if (!inRange(from) || !inRange(to)) return INDEX_OUT_OF_RANGE;
        if (!readCell(from, to).present) return EDGE_NOT_FOUND;
        writeCell(from, to, false, 0.0);
        if (!this->isDirected()) writeCell(to, from, false, 0.0);
        return OK;
    }

    std::size_t nodeCount() const { return values.size(); }

    // en un grafo no dirigido cada arista ocupa dos celdas, asi que contarlas
    // todas daria el doble. mirando solo la mitad superior de la matriz
    // (columna >= fila) cada arista se cuenta una vez, y los self-loops, que
    // caen en la diagonal, tambien quedan contados una sola vez.
    std::size_t edgeCount() const {
        std::size_t size = values.size();
        std::size_t total = 0;
        for (std::size_t row = 0; row < size; ++row) {
            for (std::size_t column = 0; column < size; ++column) {
                if (!this->isDirected() && column < row) continue;
                if (readCell(row, column).present) ++total;
            }
        }
        return total;
    }

    bool hasEdgeAt(std::size_t from, std::size_t to) const {
        if (!inRange(from) || !inRange(to)) return false;
        return readCell(from, to).present;
    }

    Status weightAt(std::size_t from, std::size_t to, double& out) const {
        if (!inRange(from) || !inRange(to)) return INDEX_OUT_OF_RANGE;
        Cell cell = readCell(from, to);
        if (!cell.present) return EDGE_NOT_FOUND;
        out = cell.weight;
        return OK;
    }

    // recorre la fila del nodo y devuelve los indices de sus vecinos.
    // en un grafo dirigido son los sucesores, es decir a donde se puede ir
    // desde ese nodo.
    Status neighborsAt(std::size_t index, DynamicArray<std::size_t>& out) const {
        if (!inRange(index)) return INDEX_OUT_OF_RANGE;
        out.clear();
        for (std::size_t column = 0; column < values.size(); ++column) {
            if (readCell(index, column).present) {
                Status status = out.pushBack(column);
                if (status != OK) return status;
            }
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
