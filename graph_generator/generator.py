import networkx as nx
import matplotlib
import simplejson as json
import argparse

def save(G, fname):
    json.dump(dict(nodes=[[n, G.node[n]] for n in G.nodes()],
                   edges=[[u, v, G.edge[u][v]] for u,v in G.edges()]),
              open(fname, 'w'), indent=2)

def load(fname):
    G = nx.DiGraph()
    d = json.load(open(fname))
    G.add_nodes_from(d['nodes'])
    G.add_edges_from(d['edges'])
    return G

parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('nodes', type=int, help='Number of nodes in graph')
parser.add_argument('edges', type=int, help='Number of eges in graph')
parser.add_argument('path', type=str, help='Output path')
args = parser.parse_args()

graph = nx.gnm_random_graph(args.nodes, args.edges)
nx.draw(graph, pos=nx.spring_layout(graph), with_labels=True)
matplotlib.pyplot.savefig(args.path + "/graph.png")
save(graph, args.path + "/graph.json")
