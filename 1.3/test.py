import random
import sys
from itertools import permutations

import subprocess
import networkx as nx

def generate_test_case(num_nodes, num_edges, weight_range=(1, 100), remove_prob=0.2):
    nodes = []
    edges = []
    commands = []

    # nodes generation
    for i in range(num_nodes):
        node_label = str(i) # since label is a string, make it numbered
        nodes.append(node_label)
        commands.append(f"NODE {node_label}")
    
    # edges generation
    possible_edges = list(permutations(nodes, 2))
    random.shuffle(possible_edges)
    
    for i in range(min(num_edges, len(possible_edges))):
        a, b = possible_edges[i]
        weight = random.randint(weight_range[0], weight_range[1])
        edges.append((a, b, weight))
        commands.append(f"EDGE {a} {b} {weight}")
    
    # probability-based
    for node in nodes[:]:
        if random.random() < remove_prob and len(nodes) > 2:
            # Удаляем узел и все связанные с ним рёбра
            commands.append(f"REMOVE NODE {node}")
            nodes.remove(node)
            edges = [(a, b, w) for a, b, w in edges if a != node and b != node]
    
    for a, b, w in edges[:]:
        if random.random() < remove_prob:
            commands.append(f"REMOVE EDGE {a} {b}")
            edges.remove((a, b, w))
    
    
    if nodes:
        root = random.choice(nodes)
        commands.append(f"DIJKSTRA {root}")
    
    return commands


# feeds commands in neetworkx to 
class Validator:
    def __init__(self):
        self.graph = nx.DiGraph()
        self.node_labels = set()
    
    def process_command(self, command):
        splits = command.strip().replace("\n", "").split()
        output = ""

        while splits:

            if splits[0] == "NODE":
                splits.pop(0)
                node = splits[0]
                splits.pop(0)
                self.graph.add_node(node)
                self.node_labels.add(node)
                continue
                
            if splits[0] == "EDGE":
                a, b, weight = splits[1], splits[2], int(splits[3])
                splits.pop(0)
                splits.pop(0)
                splits.pop(0)
                splits.pop(0)

                if a not in self.node_labels and b not in self.node_labels:
                    output += f"Unknown nodes {a} {b}\n"
                    continue
                if a not in self.node_labels:
                    output += f"Unknown node {a}\n"
                    continue
                if b not in self.node_labels:
                    output += f"Unknown node {b}\n"
                    continue
                self.graph.add_edge(a, b, weight=weight)
                continue
                
            if splits[0] == "REMOVE":
                splits.pop(0)
                if splits[0] == "NODE":
                    splits.pop(0)
                    target = splits[0]
                    splits.pop(0)

                    if target not in self.node_labels:
                        output += f"Unknown node {target}\n"
                        continue
                    self.graph.remove_node(target)
                    self.node_labels.remove(target)
                    continue

                elif splits[0] == "EDGE":
                    splits.pop(0)
                    a, b = splits[0], splits[1]
                    splits.pop(0)
                    splits.pop(0)
                    if a not in self.node_labels and b not in self.node_labels:
                        output += f"Unknown nodes {a} {b}\n"
                        continue
                    if a not in self.node_labels:
                        output += f"Unknown node {a}\n"
                        continue
                    if b not in self.node_labels:
                        output += f"Unknown node {b}\n"
                        continue

                    if not self.graph.has_edge(a, b):
                        output += f"Unknown edge {a} {b}\n"
                        continue

                    self.graph.remove_edge(a, b)
                    continue
                    
            if splits[0] == "RPO_NUMBERING":
                splits.pop(0)
                root = splits[0]
                splits.pop(0)
                if root not in self.node_labels:
                    output += f"Unknown node {root}\n"
                    continue
                
                # check for loops
                scc = list(nx.strongly_connected_components(self.graph))
                cycles = [comp for comp in scc if len(comp) > 1]
                if cycles:
                    cycle_nodes = cycles[0]
                    cycle_edges = []
                    for u in cycle_nodes:
                        for v in self.graph.successors(u):
                            if v in cycle_nodes:
                                output += f"Found loop {u}->{v}\n"
            
                order = list(nx.dfs_postorder_nodes(self.graph, source=root))
                output += " ".join(order[::-1]) + "\n"
                continue

            if splits[0] == "DIJKSTRA":
                splits.pop(0)
                src = splits[0]
                splits.pop(0)
                if src not in self.node_labels:
                    return f"Unknown node {src}\n"
                
                for drain_node in self.node_labels:
                    if drain_node == src:
                        continue
                    try:
                        length = nx.dijkstra_path_length(self.graph, src, drain_node)
                        output += f"{drain_node} {length}\n"
                    except nx.NetworkXNoPath:
                        output += f"{drain_node} inf\n" # no path found
                
                continue
                    
            if splits[0] == "MAX_FLOW":
                splits.pop(0)
                src = splits[0]
                sink = splits[1]
                splits.pop(0)
                splits.pop(0)
            
                if src not in self.node_labels and sink not in self.node_labels:
                    output += f"Unknown nodes {src} {sink}\n"
                    continue
                if src not in self.node_labels:
                    output += f"Unknown node {src}\n"
                    continue
                if sink not in self.node_labels:
                    output += f"Unknown node {sink}\n"
                    continue

                flow_value, flow_dict = nx.maximum_flow(self.graph, src, sink)
                output += flow_value + "\n"
                continue
                
            if splits[0] == "TARJAN":
                splits.pop(0)
                target = splits[0]
                splits.pop(0)
                
                if target not in self.node_labels:
                        output += f"Unknown node {target}\n"

                scc = list(nx.strongly_connected_components(self.graph))
                output += "\n".join(" ".join(comp) for comp in scc)
                continue
                    
            # may be unused
            # if splits[0] == "VISUALIZE":
            #     self.visualize_graph()
              
            # if not a command
            splits.pop(0)
    
        return output
                
    



def write_file(filename, commands):
    with open(filename, 'w') as f:
        for cmd in commands:
            f.write(cmd + '\n')


def generate_files():
    generated_files = []
    for i in range(1, 6):
        commands = generate_test_case(
            num_nodes=random.randint(3, 10),
            num_edges=random.randint(3, 20),
            remove_prob=0.3
        )
        write_file(f"tests/test_normal_{i}.txt", commands)
        generated_files += [f"tests/test_normal_{i}.txt"]
    
    # ones that are pre-defined to check basic functionality
    non_random_commands = [
        # non-existant nodes
        "NODE A",
        "EDGE A B 5",
        "REMOVE NODE X",

        "REMOVE NODE A",
        
        # non-existant edge
        "NODE A",
        "NODE B",
        "REMOVE EDGE A C",

        "REMOVE NODE A",
        "REMOVE NODE B",
    ]
    write_file("tests/test_0.txt", non_random_commands)
    generated_files += ["tests/test_0.txt"]
    
    # Генерация большого теста
    big_test = generate_test_case(
        num_nodes=25,
        num_edges=50,
        remove_prob=0.1
    )
    write_file("tests/test_big.txt", big_test)
    generated_files += ["tests/test_big.txt"]

    return generated_files



# files_list = generate_files()
files_list = ["tests/test_normal_1.txt"]

for file_path in files_list:
    validator = Validator()
    with open(file_path, 'r') as f:
        file_start = f.tell()
        process_out = subprocess.check_output(["bin/main"], stdin=f)
        print(str(process_out) + "\n")


        f.seek(file_start)
        validator_out = validator.process_command((" ".join(f.readlines())).replace("\n", ""))
        print(validator_out)