/*
 * neural_car_detector.cpp
 *
 *  Created on: 28 de jul de 2017
 *      Author: luan
 */

#include "neural_car_detector.hpp"


dbscan::Cluster
generate_cluster(std::vector<carmen_vector_3D_t> points)
{
	dbscan::Cluster cluster;
	for (unsigned int i = 0; i < points.size(); i++)
	{
		dbscan::Point point;
		point.x = points[i].x;
		point.y = points[i].y;
		cluster.push_back(point);
	}
	return (cluster);
}


dbscan::Cluster
get_biggest_cluster(dbscan::Clusters clusters)
{
	unsigned int max_size, max_index;
	max_size = 0;
	max_index = 0;
	for (unsigned int i = 0; i < clusters.size(); i++)
	{
		if (clusters[i].size() > max_size)
		{
			max_size = clusters[i].size();
			max_index = i;
		}
	}
	return (clusters[max_index]);
}


std::vector<carmen_vector_3D_t>
get_carmen_points(dbscan::Cluster cluster)
{
	std::vector<carmen_vector_3D_t> points;
	for (unsigned int i = 0; i < cluster.size(); i++)
	{
		carmen_vector_3D_t p;
		p.x = cluster[i].x;
		p.y = cluster[i].y;
		p.z = 0.0;
		points.push_back(p);
	}
	return (points);
}


carmen_vector_3D_t
translate_point(carmen_vector_3D_t point, carmen_vector_3D_t offset)
{
	point.x += offset.x;
	point.y += offset.y;
	point.z += offset.z;
	return (point);
}


carmen_vector_3D_t
rotate_point(carmen_vector_3D_t point, double theta)
{
	carmen_vector_3D_t p;
	p.x = point.x * cos(theta) - point.y * sin(theta);
	p.y = point.x * sin(theta) + point.y * cos(theta);
	p.z = point.z;
	return (p);
}


void
filter_points_in_clusters(std::vector<std::vector<carmen_vector_3D_t> > *cluster_list)
{
	for (unsigned int i = 0; i < cluster_list->size(); i++)
	{
		if ((*cluster_list)[i].size() > 0)
		{
			dbscan::Cluster cluster = generate_cluster((*cluster_list)[i]);
			dbscan::Clusters clusters = dbscan::DBSCAN(0.5, 5, cluster);
			if (clusters.size() > 0)
			{
				cluster = get_biggest_cluster(clusters);
				(*cluster_list)[i] = get_carmen_points(cluster);
			}
		}
	}
}

