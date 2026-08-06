# Grafos-EDD

Librería de grafos en C++ para trabajar con cualquier tipo de dato. Se incluye
un encabezado y se usa: no hay que compilar nada aparte, ni enlazar bibliotecas,
ni instalar dependencias.

Trae las dos representaciones clásicas —listas de adyacencia y matriz de
adyacencia— detrás de una misma interfaz, recorridos, camino más corto y varias
consultas sobre la forma del grafo.

Proyecto de la materia Estructura Dinámica de Datos.

---

## Empezar

```cpp
#include <iostream>
#include <string>
#include "graph/adjacency_list.h"
#include "graph/shortest_path.h"

int main() {
    graph::AdjacencyList<std::string> mapa;

    mapa.addNode("Maracaibo");
    mapa.addNode("Valencia");
    mapa.addNode("Caracas");

    mapa.addEdge("Maracaibo", "Valencia", 520.0);
    mapa.addEdge("Valencia", "Caracas", 160.0);
    mapa.addEdge("Maracaibo", "Caracas", 700.0);

    graph::DynamicArray<std::size_t> ruta;
    double kilometros = 0.0;

    if (graph::shortestPath(mapa, std::string("Maracaibo"),
                            std::string("Caracas"), ruta, kilometros) == graph::OK) {
        std::cout << "Distancia: " << kilometros << " km\n";
        for (std::size_t i = 0; i < ruta.size(); ++i) {
            std::size_t indice = 0;
            ruta.get(i, indice);
            std::string ciudad;
            mapa.valueAt(indice, ciudad);
            std::cout << "  " << ciudad << "\n";
        }
    }
    return 0;
}
```

Ese ejemplo devuelve 680 km pasando por Valencia, no los 700 del camino directo.
Menos aristas no significa menos distancia.

Para compilar el programa de demostración que viene con el proyecto:

```bash
g++ -std=c++11 -Wall -Wextra -Isrc src/main.cpp -o bin/grafos_demo
```

Lo único que hay que pasarle al compilador es `-Isrc`. No hay nada más que
enlazar.

---

## Estructura

```
Grafos-EDD/
├── bin/            ejecutable del demo
├── src/
│   ├── graph/      la librería
│   └── main.cpp    programa de demostración
└── README.md
```

La librería vive completa en `src/graph/`. Son solo encabezados porque las
plantillas de C++ tienen que estar disponibles en el punto donde se usan; no hay
archivos `.cpp` que compilar por separado.

| Archivo | Qué trae |
| --- | --- |
| `status.h` | códigos de error de toda la librería |
| `dynamic_array.h` | arreglo dinámico propio |
| `queue.h`, `stack.h` | cola y pila propias |
| `min_heap.h` | cola de prioridad |
| `graph_base.h` | interfaz abstracta común |
| `adjacency_list.h` | grafo por listas |
| `adjacency_matrix.h` | grafo por matriz |
| `traversal.h` | recorridos en anchura y profundidad |
| `analysis.h` | grado, componentes, conexidad, ciclos, orden topológico |
| `shortest_path.h` | camino más corto |

No se usa ningún contenedor de la biblioteca estándar. Las estructuras de apoyo
están implementadas desde cero.

---

## Qué le pide la librería a tu tipo

`T` puede ser cualquier cosa que cumpla:

- constructor por defecto
- constructor de copia y `operator=`
- `operator==`, para poder buscar un nodo por su valor
- `operator<<`, solo si vas a imprimirlo

`int`, `std::string` y cualquier `struct` propio que defina `operator==` sirven
sin más.

Los pesos son `double` y valen 1.0 si no los indicás, así que un grafo sin pesos
funciona sin tratarlo distinto. Tienen que ser finitos y no negativos: `addEdge`
rechaza los negativos, los infinitos y los que no son un número. Ese último caso
importa más de lo que parece, porque cualquier comparación contra un `NaN` da
falso y se colaría sin romper nada visible, envenenando en silencio todos los
cálculos de distancia.

---

## Elegir la representación

Las dos ofrecen exactamente la misma interfaz y los mismos resultados. Cambiar
de una a otra es cambiar el tipo que declarás.

| | Listas | Matriz |
| --- | --- | --- |
| Memoria | proporcional a las aristas | siempre nodos² |
| ¿Existe la arista? | recorre la lista del nodo | inmediato |
| Recorrer vecinos | directo | recorre la fila completa |
| Conviene con | pocas aristas | muchas aristas, o consultas constantes de adyacencia |

En la duda, listas.

Los algoritmos están escritos contra la interfaz abstracta, así que funcionan
igual con cualquiera de las dos:

```cpp
void analizar(const graph::GraphBase<std::string>& g) {
    // el mismo código sirve para listas y para matriz
}
```

---

## Dirigido o no dirigido

Se elige al construir. Por defecto es no dirigido.

```cpp
graph::AdjacencyList<std::string> sinDireccion;        // A-B implica B-A
graph::AdjacencyList<std::string> conDireccion(true);  // A->B no implica B->A
```

En un grafo no dirigido, `addEdge` registra la arista en los dos sentidos y
`edgeCount` la cuenta una sola vez.

---

## Errores

Ninguna función lanza excepciones. Todo lo que puede fallar devuelve un `Status`,
y `statusMessage` lo traduce a texto legible.

```cpp
graph::Status resultado = mapa.addEdge("Maracaibo", "Lima", 100.0);
if (resultado != graph::OK) {
    std::cout << graph::statusMessage(resultado) << "\n";  // "el nodo no existe en el grafo"
}
```

La razón es simple: una excepción sin capturar termina el programa. Con un código
de retorno el error es un valor que se puede revisar y manejar.

Una aclaración honesta: la librería no lanza **por sí misma**, ni siquiera cuando
se queda sin memoria. Lo que no puede prometer es por tu tipo. Si el constructor
de copia o el `operator=` de tu `T` lanzan, esa excepción atraviesa la librería
hasta vos. Es inevitable con plantillas: no se puede garantizar el comportamiento
de un tipo que uno no escribió.

| Código | Cuándo |
| --- | --- |
| `OK` | todo bien |
| `NODE_NOT_FOUND` | el nodo no existe |
| `NODE_ALREADY_EXISTS` | ya hay un nodo con ese valor |
| `EDGE_NOT_FOUND` | la arista no existe |
| `INDEX_OUT_OF_RANGE` | índice fuera de rango |
| `INVALID_WEIGHT` | peso negativo, infinito, o que no es un número |
| `CAPACITY_OVERFLOW` | el tamaño pedido desborda el tipo |
| `OUT_OF_MEMORY` | no hay memoria |
| `EMPTY_CONTAINER` | se pidió sacar de una cola o pila vacía |
| `EMPTY_GRAPH` | la operación necesita al menos un nodo |
| `NO_PATH` | no hay camino entre esos nodos |
| `NOT_DIRECTED` | la operación requiere un grafo dirigido |
| `HAS_CYCLE` | hay un ciclo donde no puede haberlo |

---

## Resistencia a fallos

Lo que la librería garantiza, y cómo:

**Índices fuera de rango.** El arreglo dinámico no expone `operator[]`. Todo
acceso pasa por `get` y `set`, que validan y devuelven `Status`. No hay una puerta
insegura que se pueda usar por error.

**Desbordamiento.** Antes de agrandar un arreglo se verifica que la cantidad de
elementos pedida no desborde al calcular los bytes, y que duplicar la capacidad
no dé la vuelta. En el camino más corto se comprueba que sumar un peso no pase el
límite del tipo antes de sumarlo.

**Memoria.** Toda reserva usa `new (std::nothrow)` y se verifica; si falla, se
devuelve `OUT_OF_MEMORY` en lugar de lanzar. Cada clase con punteros crudos
implementa destructor, constructor de copia y `operator=`, así que copiar un
grafo hace copia profunda y no deja dos objetos liberando la misma memoria. La
autoasignación está contemplada.

**Operaciones a medias.** Agregar una arista en un grafo no dirigido toca dos
lugares; si el segundo falla, se restaura el primero. Agregar un nodo reserva
todo lo que necesita antes de modificar nada.

**Casos borde.** Grafo vacío, nodo único, nodos aislados, self-loops, aristas de
peso cero y borrar el último nodo están cubiertos y probados.

---

## Referencia

### Grafo

```cpp
Status addNode(const T& valor);
Status removeNode(const T& valor);
Status addEdge(const T& desde, const T& hasta, double peso = 1.0);
Status removeEdge(const T& desde, const T& hasta);

std::size_t nodeCount() const;
std::size_t edgeCount() const;
std::size_t indexOf(const T& valor) const;     // NO_INDEX si no está
Status valueAt(std::size_t indice, T& salida) const;
bool hasNode(const T& valor) const;
bool hasEdge(const T& desde, const T& hasta) const;
Status weight(const T& desde, const T& hasta, double& salida) const;
bool isDirected() const;
bool empty() const;
void clear();
```

Cada función que recibe valores tiene su versión por índice terminada en `At`
(`addEdgeAt`, `weightAt`, `neighborsAt`), para cuando ya se resolvió el nodo y se
quiere evitar la búsqueda.

Agregar una arista que ya existe actualiza su peso; no la duplica. Una matriz de
adyacencia no puede representar dos aristas entre el mismo par de nodos, así que
permitirlo en listas rompería la equivalencia entre las dos implementaciones.

### Recorridos

```cpp
Status breadthFirst(const GraphBase<T>& g, const T& desde, DynamicArray<std::size_t>& orden);
Status depthFirst(const GraphBase<T>& g, const T& desde, DynamicArray<std::size_t>& orden);
```

El recorrido en profundidad es iterativo, con pila propia, para que un grafo
grande no desborde la pila de llamadas del programa.

### Análisis

```cpp
Status degree(const GraphBase<T>& g, const T& valor, std::size_t& salida);
Status connectedComponents(const GraphBase<T>& g, std::size_t& salida);
Status isConnected(const GraphBase<T>& g, bool& salida);
Status hasPath(const GraphBase<T>& g, const T& desde, const T& hasta, bool& salida);
Status hasCycle(const GraphBase<T>& g, bool& salida);
Status topologicalOrder(const GraphBase<T>& g, DynamicArray<std::size_t>& orden);
```

`connectedComponents` ignora el sentido de las aristas: dos nodos que apuntan al
mismo destino forman un solo grupo. `hasPath` sí lo respeta, porque ahí la
dirección es justamente lo que se pregunta.

`topologicalOrder` solo aplica a grafos dirigidos y devuelve `HAS_CYCLE` si no
existe un orden válido.

### Camino más corto

```cpp
Status shortestPath(const GraphBase<T>& g, const T& desde, const T& hasta,
                    DynamicArray<std::size_t>& ruta, double& distancia);

Status shortestPathUnweighted(const GraphBase<T>& g, const T& desde, const T& hasta,
                              DynamicArray<std::size_t>& ruta, std::size_t& saltos);

Status dijkstra(const GraphBase<T>& g, const T& desde,
                DynamicArray<double>& distancias,
                DynamicArray<std::size_t>& anteriores);
```

`shortestPath` usa Dijkstra y minimiza el peso total. `shortestPathUnweighted`
minimiza la cantidad de aristas mediante un recorrido en anchura. Son dos
preguntas distintas y sobre el mismo grafo pueden dar caminos distintos.

Dijkstra necesita pesos no negativos, y por eso `addEdge` rechaza los negativos
al momento de agregarlos: es mejor avisar al construir el grafo que devolver un
resultado incorrecto sin decir nada.

`dijkstra` deja las distancias a todos los nodos desde un origen. Los
inalcanzables quedan en `UNREACHABLE`.

Los parámetros de salida quedan siempre en un estado definido: si la función no
devuelve `OK`, la ruta viene vacía, la distancia en `UNREACHABLE` y los saltos en
cero. Nunca conservan lo que hubiera antes en esas variables, así que no hace
falta inicializarlas antes de llamar.

---

## Autores

Leandro Jay y Santiago Landaeta.
