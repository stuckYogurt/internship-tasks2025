#include <iostream>
#include "graph.hpp"


void splitLine(const std::string& input_line, std::queue<std::string>& request) {
    size_t start = 0;
    size_t end = input_line.find(' ');

    while (end != std::string::npos) {
        request.push(input_line.substr(start, end - start));
        start = end + 1;
        end = input_line.find(' ', start);
    }

    request.push(input_line.substr(start));
}

using namespace std;
int main() {
    std::string input_line;

    cout<< "Type \"exit\" to exit" << endl;

    // graph initialization
    Graph graph;

    while (getline(cin, input_line)) {
        if (input_line == "exit") {
            cout << "exitting...";
            return 0;
        }

        // tokenization
        std::queue<std::string> request;

        splitLine(input_line, request);


        while (!request.empty()) {
            if (request.front() == "NODE") {
                request.pop();
                graph.emplaceNode(request.front());
                request.pop();
                cout << "Created node"<<endl;
                continue;
            }

            if (request.front() == "EDGE") {
                request.pop();
                auto src_name = request.front();
                auto src = graph.getNode(src_name);
                request.pop();
                auto drain_name = request.front();
                auto drain = graph.getNode(drain_name);
                request.pop();

                auto weight = std::stoi(request.front());
                request.pop();

                if (!src && !drain) {
                    cout << "Unknown nodes " << src_name << " " << drain_name << endl;
                    continue;
                } else if (!src) {
                    cout << "Unknown node " << src_name << endl;
                    continue;
                } else if (!drain) {
                    cout << "Unknown node " << drain_name << endl;
                    continue;
                }

                graph.connect(src, drain, weight);

                cout << "Created edge" << endl;
                continue;
            }

            if (request.front() == "REMOVE") {
                request.pop();

                if (request.front() == "NODE") {
                    request.pop();
                    graph.removeNode(request.front());
                    request.pop();
                    cout << "Removed node" << endl;
                }
                else if (request.front() == "EDGE") {
                    request.pop();

                    auto src_name = request.front();
                    auto src = graph.getNode(src_name);
                    request.pop();
                    auto drain_name = request.front();
                    auto drain = graph.getNode(drain_name);
                    request.pop();

                    if (!src && !drain) {
                        cout << "Unknown nodes " << src_name << " " << drain_name << endl;
                        continue;
                    } else if (!src) {
                        cout << "Unknown node " << src_name << endl;
                        continue;
                    } else if (!drain) {
                        cout << "Unknown node " << drain_name << endl;
                        continue;
                    }

                    graph.disconnect(src, drain);
                    cout << "Removed edge" << endl;

                }
                continue;
            }

            if (request.front() == "RPO_NUMBERING") {
                request.pop();
                auto target = request.front();
                request.pop();

                if (!graph.getNode(target)) {
                    cout << "Unknown node " << target << endl;
                    continue;
                }

                graph.RPO_Numbering(target);
                continue;
            }

            request.pop(); // if command is undefined
        }
    }

}

