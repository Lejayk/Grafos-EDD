#ifndef GRAPH_GRAPH_BASE_H
#define GRAPH_GRAPH_BASE_H

#include <cstddef>
#include <cfloat>
#include "status.h"
#include "dynamic_array.h"

namespace graph {

// indice que se devuelve cuando un nodo buscado no existe.
const std::size_t NO_INDEX = static_cast<std::size_t>(-1);

// tope para el peso de una arista. el valor mas grande de un double queda
// reservado como marca de "no se puede llegar" en el camino mas corto.
const double MAX_WEIGHT = DBL_MAX;

// contrato comun a toda implementacion de grafo de esta libreria.
// de aca derivan AdjacencyList y AdjacencyMatrix, y gracias a eso los
// algoritmos de recorrido y de camino mas corto se escriben una sola vez.
//
// que le pide la libreria al tipo T:
//   - constructor por defecto
//   - constructor de copia y operator=
//   - operator== para poder buscar un nodo por su valor
//   - operator<< solo si se usan las funciones de impresion
//
// la libreria no lanza excepciones por si misma: todo lo que puede fallar de
// su lado devuelve un Status, incluidas las fallas de memoria.
//
// lo que si puede pasar es que T lance desde su propio constructor de copia o
// su operator=, y esa excepcion atraviesa la libreria hasta quien llamo. es
// inevitable con plantillas: no se puede prometer por un tipo que uno no
// escribio. si te importa que nada lance, usa un T cuyas copias no lancen.
template <typename T>
class GraphBase {
public:
    explicit GraphBase(bool directed = false) : directedGraph(directed) {}

    // virtual para que borrar por puntero a la clase base libere de verdad
    // la memoria de la implementacion concreta.
    virtual ~GraphBase() {}

    // --- construccion por valor, la forma comoda ---
    virtual Status addNode(const T& value) = 0;
    virtual Status removeNode(const T& value) = 0;
    virtual Status addEdge(const T& from, const T& to, double weight = 1.0) = 0;
    virtual Status removeEdge(const T& from, const T& to) = 0;

    // --- construccion por indice, para quien ya resolvio el nodo ---
    virtual Status addEdgeAt(std::size_t from, std::size_t to, double weight = 1.0) = 0;
    virtual Status removeEdgeAt(std::size_t from, std::size_t to) = 0;

    // --- consultas ---
    virtual std::size_t nodeCount() const = 0;
    virtual std::size_t edgeCount() const = 0;
    virtual std::size_t indexOf(const T& value) const = 0;
    virtual Status valueAt(std::size_t index, T& out) const = 0;
    virtual bool hasEdgeAt(std::size_t from, std::size_t to) const = 0;
    virtual Status weightAt(std::size_t from, std::size_t to, double& out) const = 0;

    // unico punto de acceso a la vecindad de un nodo. los algoritmos se
    // escriben contra este metodo y por eso corren igual sobre listas que
    // sobre matriz, sin saber cual tienen debajo.
    virtual Status neighborsAt(std::size_t index, DynamicArray<std::size_t>& out) const = 0;

    virtual void clear() = 0;

    // --- utilidades que no dependen de la implementacion ---
    bool isDirected() const { return directedGraph; }

    bool empty() const { return nodeCount() == 0; }

    bool hasNode(const T& value) const { return indexOf(value) != NO_INDEX; }

    bool hasEdge(const T& from, const T& to) const {
        std::size_t origin = indexOf(from);
        std::size_t target = indexOf(to);
        if (origin == NO_INDEX || target == NO_INDEX) return false;
        return hasEdgeAt(origin, target);
    }

    Status weight(const T& from, const T& to, double& out) const {
        std::size_t origin = indexOf(from);
        std::size_t target = indexOf(to);
        if (origin == NO_INDEX || target == NO_INDEX) return NODE_NOT_FOUND;
        return weightAt(origin, target, out);
    }

protected:
    // true si el grafo es dirigido. en no dirigido cada arista se registra en
    // las dos direcciones, y de eso se encarga cada implementacion.
    bool directedGraph;

    // dijkstra da resultados incorrectos con pesos negativos, asi que se
    // rechazan en la puerta de entrada en vez de fallar en silencio despues.
    //
    // la condicion esta escrita como "no es mayor o igual a cero" y no como
    // "es menor que cero" a proposito: cualquier comparacion contra un NaN da
    // false, asi que un peso NaN pasaria el filtro "weight < 0.0" y despues
    // envenenaria en silencio todas las comparaciones de distancia.
    //
    // tambien se rechazan los pesos desmedidos: la libreria usa el valor mas
    // grande de un double como marca de "inalcanzable", asi que una arista con
    // ese peso se confundiria con la ausencia de camino.
    static Status validateWeight(double weight) {
        if (!(weight >= 0.0)) return INVALID_WEIGHT;
        if (weight >= MAX_WEIGHT) return INVALID_WEIGHT;
        return OK;
    }
};

}

#endif
