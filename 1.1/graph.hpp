#include <iostream>
#include <set>
#include <stack>
#include <queue>
#include <memory>

#include <limits>

#define uns unsigned
#define EDGE_WEIGHT_T uns             // weight type for edge

enum Direction {
    No, In, Out
};

// used for colored DFS
enum Color {
    White, Gray, Black
};

class Node;

class Edge {
private:
    Node* src;
    Node* drain;

    EDGE_WEIGHT_T weight = 0;
    // index in Graph's vector edges
    uns long id;
public:

    Edge(Node* src, Node* drain, EDGE_WEIGHT_T weight, uns long id);

    // move only semantic to ensure deconstructor won't be triggered
    // deleting instance means physical deletion from graph: nodes' disconnection evoked
    Edge(const Edge&) = delete;
    Edge& operator=(const Edge&) = delete;

    Edge(Edge&& rhs) noexcept = default;
    Edge& operator=(Edge&&) = default;

    ~Edge();

    uns long getId() {return id;}

    void setId(uns long _) {id = _;}

    EDGE_WEIGHT_T getWeight() const;

    Node* getSrc();

    Node* getDrain();
};


class Node {
private:
    std::set<Edge*> InEdges;
    std::set<Edge*> OutEdges;
    std::string mark;

    // index in Graph's vector nodes
    unsigned long id;
public:
    explicit Node(std::string& mark, unsigned long id) : mark(mark), id(id) {}

    // move only semantic to ensure deconstructor won't be triggered in some cases
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    Node(Node&& rhs) noexcept = default;
    Node& operator=(Node&&) = default;

    ~Node () {
        // std::cout << "Deleting node " << mark << std::endl;
        for (auto i : InEdges)
            i->getSrc()->disconnectEdge(i, Direction::Out);

        for (auto i : OutEdges)
            i->getSrc()->disconnectEdge(i, Direction::In);
    }

    std::string getMark() const {
        return mark;
    }
    
    unsigned long getId() const {
        return id;
    }
    
    void setId(uns long _) {
        id = _;
    }

    
    void connectEdge(Edge* edge, const Direction dir) {
        if (dir == Direction::In) {
            if (InEdges.find(edge) == InEdges.end()) {
                InEdges.emplace(edge);
                return;
            }

            std::cout << "Tried connecting " << (void*)edge << "(IN) with " << mark << ", connection exists" << std::endl;
        }

        if (OutEdges.find(edge) == OutEdges.end()) {
            OutEdges.emplace(edge);
            return;
        }

        std::cout << "Tried connecting " << (void*)edge << "(OUT) with " << mark << ", connection exists" << std::endl;
    }

    // not used apart edge deletion
    void disconnectEdge(Edge* edge, const Direction dir) {
        if (dir == Direction::In) {
            if (InEdges.find(edge) != InEdges.end()) {
                InEdges.erase(edge);
                return;
            }

            std::cout << "Tried disconnecting " << (void*)edge << "(IN) from " << mark << ", connection invalid" << std::endl;
        }

        if (OutEdges.find(edge) != OutEdges.end()) {
            OutEdges.erase(edge);
            return;
        }

        std::cout << "Tried disconnecting " << (void*)edge << "(OUT) from " << mark << ", connection invalid" << std::endl;
    }

    Edge* getOutEdge(Node* target) {
        for (auto& i : OutEdges)
            if (i->getDrain() == target)
                return i;

        return nullptr;
    }

    std::set<Edge*>& getOutEdges() {
        return OutEdges;
    }

    Edge* getInEdge(Node* target) {
        for (auto& i : InEdges)
            if (i->getSrc() == target)
                return i;

        return nullptr;
    }

    std::set<Edge*>& getInEdges() {
        return InEdges;
    }

};


Edge::Edge(Node* src, Node* drain, EDGE_WEIGHT_T weight, u_long id) : src(src), drain(drain), weight(weight), id(id) {
    src->connectEdge(this, Direction::Out);
    drain->connectEdge(this, Direction::In);
}

// if being disconnected, whole instance is deleted
Edge::~Edge () {
    src->disconnectEdge(this, Direction::Out);
    drain->disconnectEdge(this, Direction::In);

    // std::cout << "Disconnected " << src->getMark() << " from " << drain->getMark() << std::endl;
}

EDGE_WEIGHT_T Edge::getWeight() const {
    return weight;
}

Node* Edge::getSrc() {
    return src;
}

Node* Edge::getDrain() {
    return drain;
}


class Graph {
private:
    // allocation directly in instance
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::unique_ptr<Edge>> edges;



public:
    Node* getNode(const std::string& mark) {
        for (auto& i : nodes)
            if (i->getMark() == mark)
                return i.get();

        return nullptr;
    }

    Node* getNode(uns long id) {
        return nodes[id].get();
    }

    size_t getNodesCount() {
        return nodes.size();
    }

    // first disconnects linked edges, then the node
    void removeNode(const std::string& mark) {
        if (!getNode(mark)) {
            std::cout<< "Unknown node " << mark << std::endl;
            return;
        }

        for (auto i = edges.begin(); i != edges.end(); i++)
            if ((*i)->getSrc()->getMark() == mark || (*i)->getDrain()->getMark() == mark) {
                edges.erase(i);
                i--;
            }

        // update edges' id
        for (uns i = 0; i < edges.size(); i++) {
            edges[i]->setId(i);
        }

        auto target_id = getNode(mark)->getId();
        nodes.erase(nodes.begin() + (signed)target_id);

        // updating ids after removing
        for (uns i = target_id; i < nodes.size(); i++) {
            nodes[i]->setId(i);
        }
    }

    void emplaceNode(std::string& mark) {
        // avoiding repeats
        if (getNode(mark)) {
            std::cout << "tried creating already existing node " << mark << std::endl;
            return;
        }

        nodes.emplace_back(new Node(mark, nodes.size()));
    }



    Edge* getEdge(Node* src, Node* drain) {
        for (auto i : src->getOutEdges())
            if (i->getDrain() == drain)
                return i;

        return nullptr;
    }

    Edge* getEdge(uns long id) {
        return edges[id].get();
    }

    size_t getEdgesCount() {
        return edges.size();
    }

    // assume input is correct: connection is new, nodes exist
    void connect(Node* src, Node* drain, EDGE_WEIGHT_T weight) {
        edges.emplace_back(new Edge(src, drain, weight, edges.size()));
    }

    void disconnect(Node* src, Node* drain) {
        if (!getEdge(src, drain)) {
            std::cout << "Unknown edge " << src->getMark() << " " << drain->getMark() << std::endl;
            return;
        }

        uns long target_id = getEdge(src, drain)->getId();
        edges.erase(edges.begin() + (signed)target_id);

        // updating ids
        for (uns i = target_id; i < edges.size(); i++) {
            edges[i]->setId(i);
        }
    }


// Topological sort
private:
    void DFS(uns long root_node, std::vector<Color>& colors, std::stack<uns long>* numbering) {
        colors[root_node] = Color::Gray;

        // check successors
        for (auto i : nodes[root_node]->getOutEdges()) {
            if (colors[i->getDrain()->getId()] == Color::White)
                DFS(i->getDrain()->getId(), colors, numbering);

            else if (colors[i->getDrain()->getId()] == Color::Gray)
                std::cout << "Found loop " << nodes[root_node]->getMark() << "->" << i->getDrain()->getMark() << std::endl;

        }

        colors[root_node] = Color::Black;
        numbering->push(root_node);
    }

public:
    void RPO_Numbering(std::string& mark) {
        auto count = nodes.size();
        std::vector<Color> colors(count, Color::White);

        // if numbering used elsewhere, return it as a pointer
        auto numbering = new std::stack<uns long>;

        DFS(getNode(mark)->getId(), colors, numbering);

        while (!numbering->empty()) {
            std::cout << nodes[numbering->top()]->getMark() << " ";
            numbering->pop();
        }

        std::cout << std::endl;
        delete numbering;
    }

};

