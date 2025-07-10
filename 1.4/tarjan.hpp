#include "graph.hpp"

// find strongly connected components
void Tarjan(Graph& graph, Node* root) {
    uns long curr_index = 0;
    std::stack<Node*> DFS_Stack;

    // -1 means undefined
    std::vector<long long> indexes(graph.getNodesCount(), -1);
    std::vector<long long> lowlink_indexes(graph.getNodesCount(), -1);
    std::vector<bool> isOnStack(graph.getNodesCount(), false);

    std::function<void(Node*)> strongConnect =
            [&strongConnect, &graph, &DFS_Stack, &curr_index, &indexes, &lowlink_indexes, &isOnStack] (Node* node) -> void {
        indexes[node->getId()] = (signed long long)curr_index;
        lowlink_indexes[node->getId()] = (signed long long)curr_index;
        curr_index++;

        DFS_Stack.push(node);
        isOnStack[node->getId()] = true;

        // successors' processing
        for (auto i : node->getOutEdges()) {
            // undefined vertex (not yet visited)
            if (indexes[i->getDrain()->getId()] == -1) {
                strongConnect(i->getDrain());
                lowlink_indexes[node->getId()] = std::min(lowlink_indexes[node->getId()],
                                                          lowlink_indexes[i->getDrain()->getId()]);
            }
            else if (isOnStack[i->getDrain()->getId()]) {
                lowlink_indexes[node->getId()] = std::min(lowlink_indexes[node->getId()], lowlink_indexes[i->getDrain()->getId()]);
            }
        }

        // if node is root, it must lead to SCC
        if (lowlink_indexes[node->getId()] == indexes[node->getId()]) {
            Node* curr_node = nullptr;
            std::vector<Node*> outputSCC;

            do {
                curr_node = DFS_Stack.top();
                DFS_Stack.pop();

                isOnStack[curr_node->getId()] = false;
                outputSCC.push_back(curr_node);
            } while (curr_node != node);

            if (outputSCC.size() > 1) {
                for (auto i : outputSCC) 
                    std::cout << i->getMark() << " ";
                
                std::cout << std::endl;
            }
        }
    };


    // since the starting point (root) is given, no need to check unconnected graphs
    strongConnect(root);
}
