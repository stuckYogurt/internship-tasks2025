#include "graph.hpp"

std::vector<Edge*>* findPath(Graph& graph, Node* src, Node* drain, std::vector<EDGE_WEIGHT_T>& flow) {
    std::queue<Node*> Q;
    std::vector<Edge*> parents(graph.getNodesCount(), nullptr); // which edge lead to the Node
    std::vector<bool> visited(graph.getNodesCount(), false);
    visited[src->getId()] = true;

    Q.push(src);
    while (!Q.empty()) {
        auto curr_node = Q.front();
        Q.pop();

        // found drain
        if (curr_node == drain) {
            auto path = new std::vector<Edge*>;
            auto iter = curr_node;

            while (parents[iter->getId()]) {
                path->push_back(parents[iter->getId()]);
                iter = parents[iter->getId()]->getSrc();
            }

            return path;
        }

        for (auto i : curr_node->getOutEdges())
            if (!visited[i->getDrain()->getId()] && flow[i->getId()] > 0) {
                visited[i->getDrain()->getId()] = true;
                parents[i->getDrain()->getId()] = i;

                Q.push(i->getDrain());
            }
    }

    // path not found
    return nullptr;
}

EDGE_WEIGHT_T long maxFlow(Graph& graph, Node* src, Node* drain) {
    std::vector<EDGE_WEIGHT_T> flow(graph.getEdgesCount());
    std::vector<EDGE_WEIGHT_T> resulting_flow(graph.getEdgesCount(), 0);

    for (uns i = 0; i < graph.getEdgesCount(); i++)
        flow[i] = graph.getEdge(i)->getWeight();

    auto path = findPath(graph, src, drain, flow);
    while (path) {
        EDGE_WEIGHT_T min_pathFlow = (*path)[0]->getWeight();
        for (auto i : *path)
            min_pathFlow = std::min(min_pathFlow, i->getWeight());

        for (auto i : *path) {
            flow[i->getId()] -= std::max(min_pathFlow, 0u);
            resulting_flow[i->getId()] += min_pathFlow;
        }

        delete path;
        path = findPath(graph, src, drain, flow);
    }

    // total flow could be measured by resulting flow of outgoing source edges
    EDGE_WEIGHT_T total_flow = 0;
    for (auto i : src->getOutEdges())
        total_flow += resulting_flow[i->getId()];

    return total_flow;
}