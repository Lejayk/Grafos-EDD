// programa de demostracion de la libreria de grafos.
//
// no pide datos por archivo ni por teclado: los grafos se arman en codigo, que
// es como se usa una libreria de verdad. el que la incluye pone sus propios
// datos.
//
// compilar desde la raiz del proyecto:
//   g++ -std=c++11 -Wall -Wextra -Isrc src/main.cpp -o bin/grafos_demo

#include <iostream>
#include <string>
#include "graph/adjacency_list.h"
#include "graph/adjacency_matrix.h"
#include "graph/traversal.h"
#include "graph/analysis.h"
#include "graph/shortest_path.h"

using namespace graph;

static void title(const char* text) {
    std::cout << "\n=== " << text << " ===\n";
}

// imprime los valores de los nodos de una lista de indices
static void printNodes(const GraphBase<std::string>& graph,
                       const DynamicArray<std::size_t>& indices,
                       const char* separator) {
    for (std::size_t i = 0; i < indices.size(); ++i) {
        std::size_t index = 0;
        indices.get(i, index);
        std::string value;
        graph.valueAt(index, value);
        if (i > 0) std::cout << separator;
        std::cout << value;
    }
    std::cout << "\n";
}

// mapa de ciudades con las distancias en kilometros. no dirigido: si se puede
// ir de una a otra, tambien se puede volver.
//
// los numeros estan elegidos para que el camino directo Maracaibo-Caracas sea
// el de menos tramos pero NO el mas corto en distancia. sirve para mostrar que
// son dos preguntas distintas.
static void buildCityMap(GraphBase<std::string>& map) {
    map.addNode("Maracaibo");
    map.addNode("Valencia");
    map.addNode("Caracas");
    map.addNode("Barquisimeto");
    map.addNode("Merida");

    map.addEdge("Maracaibo", "Barquisimeto", 320.0);
    map.addEdge("Barquisimeto", "Valencia", 220.0);
    map.addEdge("Valencia", "Caracas", 160.0);
    map.addEdge("Maracaibo", "Caracas", 780.0);
    map.addEdge("Maracaibo", "Merida", 340.0);
}

static void showGraph(const GraphBase<std::string>& graph) {
    std::cout << graph.nodeCount() << " nodos, " << graph.edgeCount() << " aristas";
    std::cout << (graph.isDirected() ? " (dirigido)\n" : " (no dirigido)\n");

    DynamicArray<std::size_t> neighbors;
    for (std::size_t i = 0; i < graph.nodeCount(); ++i) {
        std::string value;
        graph.valueAt(i, value);
        std::cout << "  " << value << " -> ";
        graph.neighborsAt(i, neighbors);
        if (neighbors.empty()) {
            std::cout << "(sin vecinos)\n";
            continue;
        }
        printNodes(graph, neighbors, ", ");
    }
}

static void demoTraversals(const GraphBase<std::string>& map) {
    title("Recorridos desde Maracaibo");
    DynamicArray<std::size_t> order;

    if (breadthFirst(map, std::string("Maracaibo"), order) == OK) {
        std::cout << "En anchura:     ";
        printNodes(map, order, " -> ");
    }
    if (depthFirst(map, std::string("Maracaibo"), order) == OK) {
        std::cout << "En profundidad: ";
        printNodes(map, order, " -> ");
    }
}

static void demoShortestPath(const GraphBase<std::string>& map) {
    title("Camino mas corto: Maracaibo a Caracas");

    DynamicArray<std::size_t> route;
    double kilometers = 0.0;
    if (shortestPath(map, std::string("Maracaibo"), std::string("Caracas"),
                     route, kilometers) == OK) {
        std::cout << "Por distancia (" << kilometers << " km): ";
        printNodes(map, route, " -> ");
    }

    std::size_t hops = 0;
    if (shortestPathUnweighted(map, std::string("Maracaibo"), std::string("Caracas"),
                               route, hops) == OK) {
        std::cout << "Por cantidad de tramos (" << hops << "): ";
        printNodes(map, route, " -> ");
    }

    std::cout << "\nLos dos caminos son distintos, y los dos son correctos:\n";
    std::cout << "uno responde cual es el viaje mas corto, el otro cual tiene\n";
    std::cout << "menos escalas. Son preguntas diferentes.\n";

    title("Distancias desde Maracaibo a todas las ciudades");
    DynamicArray<double> distances;
    DynamicArray<std::size_t> previous;
    if (dijkstra(map, std::string("Maracaibo"), distances, previous) == OK) {
        for (std::size_t i = 0; i < map.nodeCount(); ++i) {
            std::string value;
            map.valueAt(i, value);
            double distance = 0.0;
            distances.get(i, distance);
            std::cout << "  " << value << ": ";
            if (distance == UNREACHABLE) std::cout << "no se puede llegar\n";
            else std::cout << distance << " km\n";
        }
    }
}

static void demoAnalysis(const GraphBase<std::string>& map) {
    title("Analisis del mapa");

    std::size_t number = 0;
    if (degree(map, std::string("Maracaibo"), number) == OK) {
        std::cout << "Maracaibo conecta con " << number << " ciudades\n";
    }
    if (connectedComponents(map, number) == OK) {
        std::cout << "Grupos aislados entre si: " << number << "\n";
    }

    bool flag = false;
    if (isConnected(map, flag) == OK) {
        std::cout << "Se puede ir de cualquier ciudad a cualquier otra: "
                  << (flag ? "si" : "no") << "\n";
    }
    if (hasCycle(map, flag) == OK) {
        std::cout << "Hay rutas alternativas (ciclos): " << (flag ? "si" : "no") << "\n";
    }
    if (hasPath(map, std::string("Merida"), std::string("Caracas"), flag) == OK) {
        std::cout << "Hay camino de Merida a Caracas: " << (flag ? "si" : "no") << "\n";
    }
}

// grafo dirigido de dependencias entre tareas: la flecha significa
// "hay que hacer esto antes que aquello".
static void demoTopologicalOrder() {
    title("Orden de tareas (grafo dirigido)");

    AdjacencyList<std::string> tasks(true);
    tasks.addNode("disenar");
    tasks.addNode("programar");
    tasks.addNode("probar");
    tasks.addNode("documentar");
    tasks.addNode("entregar");

    tasks.addEdge("disenar", "programar");
    tasks.addEdge("programar", "probar");
    tasks.addEdge("programar", "documentar");
    tasks.addEdge("probar", "entregar");
    tasks.addEdge("documentar", "entregar");

    DynamicArray<std::size_t> order;
    if (topologicalOrder(tasks, order) == OK) {
        std::cout << "Orden valido: ";
        printNodes(tasks, order, " -> ");
    }

    // se agrega una dependencia circular y el algoritmo la detecta
    tasks.addEdge("entregar", "disenar");
    Status status = topologicalOrder(tasks, order);
    std::cout << "\nAgregando 'entregar' antes de 'disenar' se cierra un circulo:\n";
    std::cout << "  " << statusMessage(status) << "\n";
}

// la razon de ser de la interfaz abstracta: el mismo codigo, dos estructuras
// distintas por debajo, resultados identicos.
static void demoBothImplementations() {
    title("La misma pregunta sobre las dos implementaciones");

    AdjacencyList<std::string> withLists;
    AdjacencyMatrix<std::string> withMatrix;
    buildCityMap(withLists);
    buildCityMap(withMatrix);

    GraphBase<std::string>* implementations[2];
    implementations[0] = &withLists;
    implementations[1] = &withMatrix;
    const char* names[2] = {"listas de adyacencia", "matriz de adyacencia"};

    for (int i = 0; i < 2; ++i) {
        DynamicArray<std::size_t> route;
        double kilometers = 0.0;
        shortestPath(*implementations[i], std::string("Maracaibo"),
                     std::string("Caracas"), route, kilometers);
        std::cout << "  " << names[i] << ": " << kilometers << " km, ";
        printNodes(*implementations[i], route, " -> ");
    }

    std::cout << "\nNi shortestPath ni ningun otro algoritmo sabe cual de las dos\n";
    std::cout << "estructuras tiene debajo. Estan escritos una sola vez contra la\n";
    std::cout << "interfaz abstracta.\n";
}

static void demoErrorHandling() {
    title("Manejo de errores");

    AdjacencyList<std::string> graph;
    graph.addNode("A");
    graph.addNode("B");

    std::cout << "Agregar un nodo repetido:      " << statusMessage(graph.addNode("A")) << "\n";
    std::cout << "Arista a un nodo inexistente:  "
              << statusMessage(graph.addEdge("A", "Z", 1.0)) << "\n";
    std::cout << "Arista con peso negativo:      "
              << statusMessage(graph.addEdge("A", "B", -5.0)) << "\n";

    std::string value;
    std::cout << "Leer un indice fuera de rango: "
              << statusMessage(graph.valueAt(99, value)) << "\n";

    DynamicArray<std::size_t> route;
    double distance = 0.0;
    std::cout << "Camino entre nodos sin unir:   "
              << statusMessage(shortestPathAt(graph, 0, 1, route, distance)) << "\n";

    DynamicArray<std::size_t> order;
    std::cout << "Orden topologico en no dirigido: "
              << statusMessage(topologicalOrder(graph, order)) << "\n";

    std::cout << "\nNinguna de estas operaciones interrumpe el programa. Todas\n";
    std::cout << "devuelven un codigo que el que llama puede revisar.\n";
}

int main() {
    std::cout << "Libreria de grafos - programa de demostracion\n";

    AdjacencyList<std::string> map;
    buildCityMap(map);

    title("Mapa de ciudades");
    showGraph(map);

    demoTraversals(map);
    demoShortestPath(map);
    demoAnalysis(map);
    demoTopologicalOrder();
    demoBothImplementations();
    demoErrorHandling();

    std::cout << "\nPresione Enter para salir...";
    std::cin.get();
    return 0;
}
