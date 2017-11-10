/*
 * virtual_scan.h
 *
 *  Created on: Nov 09, 2017
 *      Author: claudine
 */

#ifndef SRC_VIRTUAL_SCAN_VIRTUAL_SCAN_NEIGHBORHOOD_GRAPH_H_
#define SRC_VIRTUAL_SCAN_VIRTUAL_SCAN_NEIGHBORHOOD_GRAPH_H_


typedef struct
{
	int num_pointers;
	void **pointers;
} virtual_scan_elements_t;


typedef struct _virtual_scan_graph_node_t
{
	virtual_scan_box_model_t box_model;
	double timestamp;
	virtual_scan_elements_t parents;
	virtual_scan_elements_t children;
	virtual_scan_elements_t siblings;
} virtual_scan_graph_node_t;


typedef struct
{
	int num_graph_nodes;
	virtual_scan_graph_node_t *graph_nodes;
} virtual_scan_complete_sub_graph_t;


typedef struct _virtual_scan_disconnected_sub_graph_t
{
	double timestamp;
	int num_complete_sub_graphs; // num_graphs
	virtual_scan_complete_sub_graph_t *complete_sub_graphs; // graphs
	_virtual_scan_disconnected_sub_graph_t *previous_disconnected_sub_graph; //previous
	_virtual_scan_disconnected_sub_graph_t *next;
} virtual_scan_disconnected_sub_graph_t;


typedef struct
{
	virtual_scan_disconnected_sub_graph_t *disconnected_sub_graph; // graph
	int subgraph_index; //
	int node_index;
} virtual_scan_disconnected_sub_graph_iterator_t;


typedef struct
{
	int num_disconnected_sub_graphs; // num_graphs
	virtual_scan_disconnected_sub_graph_t *first_disconnected_sub_graph; //first
	virtual_scan_disconnected_sub_graph_t *last_disconnected_sub_graph; // last
} virtual_scan_neighborhood_graph_t;


virtual_scan_neighborhood_graph_t *
virtual_scan_alloc_neighborhood_graph ();


void
update_neighborhood_graph (
		virtual_scan_neighborhood_graph_t *neighborhood_graph,
		virtual_scan_box_model_hypotheses_t *box_model_hypotheses
);


#endif /* SRC_VIRTUAL_SCAN_VIRTUAL_SCAN_NEIGHBORHOOD_GRAPH_H_ */
