import random
import sys
from itertools import permutations

def generate_test_case(num_nodes, num_edges, remove_prob=0.2):
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
        weight = 1
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
        commands.append(f"RPO_NUMBERING {root}")
    
    return commands


def write_file(filename, commands):
    with open(filename, 'w') as f:
        for cmd in commands:
            f.write(cmd + '\n')


def main():
    # Генерация нормальных тестов
    for i in range(1, 6):
        commands = generate_test_case(
            num_nodes=random.randint(3, 10),
            num_edges=random.randint(3, 20),
            remove_prob=0.3
        )
        write_file(f"tests/test_normal_{i}.txt", commands)
    
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
        
        # cyclical graph
        "NODE A",
        "NODE B",
        "NODE C",
        "EDGE A B 1",
        "EDGE B C 2",
        "EDGE C A 3",
        "RPO_NUMBERING A"
    ]
    write_file("tests/test_0.txt", non_random_commands)
    
    # Генерация большого теста
    big_test = generate_test_case(
        num_nodes=25,
        num_edges=50,
        remove_prob=0.1
    )
    write_file("tests/test_big.txt", big_test)

if __name__ == "__main__":
    main()