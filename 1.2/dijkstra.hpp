#include <limits>
#include "graph.hpp"


void Dijkstra_path(Graph& graph, Node* root_node) {
    auto node_count = graph.getNodesCount();

    // if distances are needed elsewhere, return it w/o freeing
    auto distances = new std::vector<EDGE_WEIGHT_T>(node_count, std::numeric_limits<EDGE_WEIGHT_T>::max());
    std::vector<bool> visited(node_count, false); // could be changed to bitset if large graphs are at use

    (*distances)[root_node->getId()] = 0;

    for (uns _ = 0; _ < node_count; _++) {
        long v = -1;
        for (uns u = 0; u < node_count; u++)
            if (!visited[u] && (v==-1 || (*distances)[u] < (*distances)[v]))
                v = u;

        //
        if (v == -1) {
            break;
        }

        visited[v] = true;
        auto edges = graph.getNode(v)->getOutEdges();
        for (auto e : edges)
            (*distances)[e->getDrain()->getId()] = std::min((*distances)[e->getDrain()->getId()],
                (*distances)[v] == std::numeric_limits<EDGE_WEIGHT_T>::max() ? std::numeric_limits<EDGE_WEIGHT_T>::max() : (*distances)[v] + e->getWeight());

    }

    for (uns i = 0; i < node_count; i++) {
        if (i == root_node->getId()) continue;

        std::cout << graph.getNode(i)->getMark() << " " <<
        ((*distances)[i] == std::numeric_limits<EDGE_WEIGHT_T>::max() ? "inf" : std::to_string((*distances)[i])) <<
        std::endl;
    }

    delete distances;
}