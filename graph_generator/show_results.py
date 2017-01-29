import networkx as nx
import matplotlib
import simplejson as json
import argparse

def load(fname):
    G = nx.Graph()
    d = json.load(open(fname))
    G.add_nodes_from(d['nodes'])
    for edge in d['edges']:
        G.add_edge(edge[0], edge[1], weight=edge[2])
    return G

parser = argparse.ArgumentParser(description='Draw weighted graph.')
parser.add_argument('input_json', type=str, help='Path to json file to be read')
parser.add_argument('output_graph', type=str, help='Path to file to save graph')
args = parser.parse_args()

graph = load(args.input_json)
pos=nx.spring_layout(graph)
nx.draw_networkx_nodes(graph, pos, node_size=700)

elarge=[(u,v) for (u,v,d) in graph.edges(data=True) if d['weight'] >10]
esmall=[(u,v) for (u,v,d) in graph.edges(data=True) if d['weight'] <=10]
nx.draw_networkx_edges(graph,pos,edgelist=elarge, width=6)
nx.draw_networkx_edges(graph,pos,edgelist=esmall, width=6,alpha=0.5,edge_color='b',style='dashed')

nx.draw_networkx_labels(graph,pos,font_size=20,font_family='sans-serif')


matplotlib.pyplot.savefig(args.output_graph)
