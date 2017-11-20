/*
 * tracker.h
 *
 *  Created on: Nov 14, 2017
 *      Author: claudine
 */

#ifndef SRC_VIRTUAL_SCAN_TRACKER_H_
#define SRC_VIRTUAL_SCAN_TRACKER_H_

#include <vector>
#include <random>
#include "virtual_scan_neighborhood_graph.h"

namespace virtual_scan
{

class Track
{
	std::deque<virtual_scan_graph_node_t *> graph_nodes;

public:
	~Track();
	int size();
	void append_front(virtual_scan_graph_node_t *node);
	void append_back(virtual_scan_graph_node_t *node);
	virtual_scan_graph_node_t *front_node();
	virtual_scan_graph_node_t *back_node();
	void track_forward_reduction(int r);
	void track_backward_reduction(int r);
};

class Tracks
{
	std::vector<Track> tracks;
	std::random_device *rd;

	bool track_extension(virtual_scan_neighborhood_graph_t *neighborhood_graph, Track *tau);
	bool track_extension(virtual_scan_neighborhood_graph_t *neighborhood_graph);
	bool track_forward_extension(Track *tau);
	bool track_backward_extension(Track *tau);
	void track_reduction(virtual_scan_neighborhood_graph_t *neighborhood_graph);
	bool track_birth(virtual_scan_neighborhood_graph_t *neighborhood_graph);
	bool track_death();
	void track_split(virtual_scan_neighborhood_graph_t *neighborhood_graph);
	void track_merge(virtual_scan_neighborhood_graph_t *neighborhood_graph);
	void track_switch(virtual_scan_neighborhood_graph_t *neighborhood_graph);
	void track_diffusion(virtual_scan_neighborhood_graph_t *neighborhood_graph);
public:
	Tracks(std::random_device *rd);
	Tracks *propose(virtual_scan_neighborhood_graph_t *neighborhood_graph);
	double P();
};

class Tracker
{
	int n_mc;
	virtual_scan_neighborhood_graph_t *neighborhood_graph;
	Tracks *tracks_n;
	Tracks *tracks_star;

	std::random_device rd;
	std::uniform_real_distribution<> uniform;

	bool keep(Tracks *new_tracks);

public:
	Tracker(int n);
	Tracks *track(virtual_scan_box_model_hypotheses_t *box_model_hypotheses);
};

} /* namespace virtual_scan */


#endif /* SRC_VIRTUAL_SCAN_TRACKER_H_ */



