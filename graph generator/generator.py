import networkx as nx
import matplotlib
import simplejson as json

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

graph = nx.gnm_random_graph(15, 25)
nx.draw(graph, pos=nx.spring_layout(graph), with_labels=True)
matplotlib.pyplot.savefig("graph.png")
save(graph, "graph.json")