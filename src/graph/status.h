#ifndef GRAPH_STATUS_H
#define GRAPH_STATUS_H

namespace graph {

// resultado de toda operacion que puede fallar. la libreria no lanza
// excepciones a proposito: una excepcion sin capturar tumba el programa
// entero, que es justo lo contrario a ser resistente a fallos. aca el error
// es un valor que quien llama puede revisar y manejar.
enum Status {
    OK = 0,
    NODE_NOT_FOUND,
    NODE_ALREADY_EXISTS,
    EDGE_NOT_FOUND,
    INDEX_OUT_OF_RANGE,
    INVALID_WEIGHT,
    CAPACITY_OVERFLOW,
    OUT_OF_MEMORY,
    EMPTY_CONTAINER,
    EMPTY_GRAPH,
    NO_PATH,
    NOT_DIRECTED,
    HAS_CYCLE
};

// texto listo para mostrarle al usuario final.
// inline porque la libreria es header-only y este archivo se incluye desde
// varias unidades de compilacion.
inline const char* statusMessage(Status status) {
    switch (status) {
        case OK:                  return "operacion exitosa";
        case NODE_NOT_FOUND:      return "el nodo no existe en el grafo";
        case NODE_ALREADY_EXISTS: return "el nodo ya existe en el grafo";
        case EDGE_NOT_FOUND:      return "la arista no existe en el grafo";
        case INDEX_OUT_OF_RANGE:  return "indice fuera de rango";
        case INVALID_WEIGHT:      return "peso invalido: no se admiten pesos negativos";
        case CAPACITY_OVERFLOW:   return "desbordamiento de capacidad";
        case OUT_OF_MEMORY:       return "no hay memoria disponible";
        case EMPTY_CONTAINER:     return "el contenedor esta vacio";
        case EMPTY_GRAPH:         return "el grafo esta vacio";
        case NO_PATH:             return "no existe camino entre los nodos indicados";
        case NOT_DIRECTED:        return "la operacion requiere un grafo dirigido";
        case HAS_CYCLE:           return "el grafo contiene un ciclo";
    }
    return "estado desconocido";
}

}

#endif
