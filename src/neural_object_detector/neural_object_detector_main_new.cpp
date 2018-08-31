#include <carmen/carmen.h>
#include <carmen/bumblebee_basic_interface.h>
#include <carmen/velodyne_interface.h>
#include <carmen/velodyne_camera_calibration.h>
#include <carmen/camera_boxes_to_world.h>
#include <carmen/moving_objects_messages.h>
#include <carmen/moving_objects_interface.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <opencv2/highgui/highgui.hpp>
#include "Darknet.hpp"
#include <carmen/dbscan.h>

using namespace std;
using namespace cv;

int camera;
int camera_side;

carmen_camera_parameters camera_parameters;
carmen_pose_3D_t velodyne_pose;
carmen_pose_3D_t camera_pose;

Detector *darknet;
vector<string> obj_names_vector;
vector<Scalar> obj_colours_vector;

carmen_velodyne_partial_scan_message *velodyne_msg;
carmen_point_t globalpos;


void
objects_names_from_file(string const class_names_file)
{
    ifstream file(class_names_file);

    if (!file.is_open())
    	return;

    for (string line; getline(file, line);)
    	obj_names_vector.push_back(line);

    cout << "Object names loaded!\n\n";
}


void
set_object_vector_color()
{
	for (unsigned int i = 0; i < obj_names_vector.size(); i++)
	{
		if (i == 2 ||                                         // car
			i == 3 ||                                         // motorbike
			i == 5 ||                                         // bus
			i == 6 ||                                         // train
			i == 7)                                           // truck
			obj_colours_vector.push_back(Scalar(0, 0, 255));
		else if (i == 0 || i == 1)                            // person or bicycle
			obj_colours_vector.push_back(Scalar(255, 0, 0));
		else
			obj_colours_vector.push_back(Scalar(230, 230, 230));
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Publishers                                                                                //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////


void
publish_moving_objects_message(double timestamp, carmen_moving_objects_point_clouds_message *msg)
{
	msg->timestamp = timestamp;
	msg->host = carmen_get_host();

    carmen_moving_objects_point_clouds_publish_message(msg);
}


///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Handlers                                                                                  //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////


vector<vector<image_cartesian>>
filter_points_inside_bounding_boxes(vector<bbox_t> &predictions, vector<image_cartesian> &velodyne_points_vector)
{
	vector<vector<image_cartesian>> laser_list_inside_each_bounding_box; //each_bounding_box_laser_list

	for (unsigned int i = 0; i < predictions.size(); i++)
	{
		vector<image_cartesian> lasers_points_inside_bounding_box;

		for (unsigned int j = 0; j < velodyne_points_vector.size(); j++)
		{
			if (velodyne_points_vector[j].image_x >  predictions[i].x &&
				velodyne_points_vector[j].image_x < (predictions[i].x + predictions[i].w) &&
				velodyne_points_vector[j].image_y >  predictions[i].y &&
				velodyne_points_vector[j].image_y < (predictions[i].y + predictions[i].h))
			{
				lasers_points_inside_bounding_box.push_back(velodyne_points_vector[j]);
			}
		}
		laser_list_inside_each_bounding_box.push_back(lasers_points_inside_bounding_box);
	}
	return laser_list_inside_each_bounding_box;
}


vector<image_cartesian>
get_biggest_cluster(vector<vector<image_cartesian>> &clusters)
{
	unsigned int max_size = 0, max_index = 0;

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


inline double
distance2(image_cartesian a, image_cartesian b)
{
	double dx = a.cartesian_x - b.cartesian_x;
	double dy = a.cartesian_y - b.cartesian_y;

	return (dx * dx + dy * dy);
}


vector<int>
query(double d2, int i, const vector<image_cartesian> &points, std::vector<bool> clustered)
{
	vector<int> neighbors;
	const image_cartesian &point = points[i];

	for (size_t j = 0; j < points.size(); j++)
	{
		if ((distance2(point, points[j]) < d2) && !clustered[j])
			neighbors.push_back(j);
	}

	return (neighbors);
}


vector<vector<image_cartesian>>
dbscan_compute_clusters(double d2, size_t density, const vector<image_cartesian> &points)
{
	vector<vector<image_cartesian>> clusters;
	vector<bool> clustered(points.size(), false);

	for (size_t i = 0; i < points.size(); ++i)
	{
		// Ignore already clustered points.
		if (clustered[i])
			continue;

		// Ignore points without enough neighbors.
		vector<int> neighbors = query(d2, i, points, clustered);
		if (neighbors.size() < density)
			continue;

		// Create a new cluster with the i-th point as its first element.
		vector<image_cartesian> c;
		clusters.push_back(c);
		vector<image_cartesian> &cluster = clusters.back();
		cluster.push_back(points[i]);
		clustered[i] = true;

		// Add the point's neighbors (and possibly their neighbors) to the cluster.
		for (size_t j = 0; j < neighbors.size(); ++j)
		{
			int k = neighbors[j];
			if (clustered[k])
				continue;

			cluster.push_back(points[k]);
			clustered[k] = true;

			vector<int> farther = query(d2, k, points, clustered);
			if (farther.size() >= density)
				neighbors.insert(neighbors.end(), farther.begin(), farther.end());
		}
	}
	return (clusters);
}


vector<vector<image_cartesian>>
filter_object_points_using_dbscan(vector<vector<image_cartesian>> &points_lists)
{
	vector<vector<image_cartesian>> filtered_points;

	for (unsigned int i = 0; i < points_lists.size(); i++)
	{
		vector<vector<image_cartesian>> clusters = dbscan_compute_clusters(0.5, 3, points_lists[i]);        // Compute clusters using dbscan

		if (clusters.size() == 0)                                          // If dbscan returned zero clusters
		{
			vector<image_cartesian> empty_cluster;
			filtered_points.push_back(empty_cluster);                      // An empty cluster is added to the clusters vector
		}
		else if (clusters.size() == 1)
		{
			filtered_points.push_back(clusters[0]);
		}
		else                                                               // dbscan returned more than one cluster
		{
			filtered_points.push_back(get_biggest_cluster(clusters));      // get the biggest, the biggest cluster will always better represent the object
		}
	}
	return (filtered_points);
}


void
show_LIDAR_points(Mat &image, vector<image_cartesian> all_points)
{
	for (unsigned int i = 0; i < all_points.size(); i++)
		circle(image, Point(all_points[i].image_x, all_points[i].image_y), 1, cvScalar(0, 0, 255), 1, 8, 0);
}


void
show_LIDAR(Mat &image, vector<vector<image_cartesian>> points_lists, int r, int g, int b)
{
	for (unsigned int i = 0; i < points_lists.size(); i++)
	{
		for (unsigned int j = 0; j < points_lists[i].size(); j++)
			circle(image, Point(points_lists[i][j].image_x, points_lists[i][j].image_y), 1, cvScalar(b, g, r), 1, 8, 0);
	}
}


void
show_all_points(Mat &image, unsigned int crop_x, unsigned int crop_y, unsigned int crop_width, unsigned int crop_height)
{
	vector<carmen_velodyne_points_in_cam_t> all_points = carmen_velodyne_camera_calibration_lasers_points_in_camera(
					velodyne_msg, camera_parameters, velodyne_pose, camera_pose, 640, 480);

	int max_x = crop_x + crop_width, max_y = crop_y + crop_height;

	for (unsigned int i = 0; i < all_points.size(); i++)
		if (all_points[i].ipx >= crop_x && all_points[i].ipx <= max_x && all_points[i].ipy >= crop_y && all_points[i].ipy <= max_y)
			circle(image, Point(all_points[i].ipx - crop_x, all_points[i].ipy - crop_y), 1, cvScalar(0, 0, 255), 1, 8, 0);
}


void
show_detections(Mat image, vector<bbox_t> predictions, vector<image_cartesian> points, vector<vector<image_cartesian>> points_inside_bbox,
		vector<vector<image_cartesian>> filtered_points, double fps, unsigned int crop_x, unsigned int crop_y, unsigned int crop_width, unsigned int crop_height)
{
	char object_info[25];
    char frame_rate[25];

    cvtColor(image, image, COLOR_RGB2BGR);

    sprintf(frame_rate, "FPS = %.2f", fps);

    putText(image, frame_rate, Point(10, 25), FONT_HERSHEY_PLAIN, 2, cvScalar(0, 255, 0), 2);

    for (unsigned int i = 0; i < predictions.size(); i++)
    {
        sprintf(object_info, "%d %s %.2f", predictions[i].obj_id, obj_names_vector[predictions[i].obj_id].c_str(), predictions[i].prob);

        rectangle(image, Point(predictions[i].x, predictions[i].y), Point((predictions[i].x + predictions[i].w), (predictions[i].y + predictions[i].h)),
                      obj_colours_vector[predictions[i].obj_id], 1);

        putText(image, object_info, Point(predictions[i].x + 1, predictions[i].y - 3), FONT_HERSHEY_PLAIN, 1, cvScalar(255, 255, 0), 1);
    }

	show_all_points(image, crop_x, crop_y, crop_width, crop_height);
    show_LIDAR(image, points_inside_bbox,    0, 0, 255);				// Blue points are all points inside the bbox
    show_LIDAR(image, filtered_points, 0, 255, 0); 						// Green points are filtered points

    resize(image, image, Size(640, 480));
    imshow("Neural Object Detector", image);
    //imwrite("Image.jpg", image);
    waitKey(1);
}


vector<image_cartesian>
compute_detected_objects_poses(vector<vector<image_cartesian>> filtered_points)
{
	vector<image_cartesian> objects_positions;

	for(unsigned int i = 0; i < filtered_points.size(); i++)
	{
		image_cartesian point;

		if (filtered_points[i].size() == 0)
		{
			point.cartesian_x = -999.0;    // This error code is set, probably the object is out of the LiDAR's range
			point.cartesian_y = -999.0;
			point.cartesian_z = -999.0;
		}
		else
		{
			point.cartesian_x = 0.0;
			point.cartesian_y = 0.0;
			point.cartesian_z = 0.0;

			for(unsigned int j = 0; j < filtered_points[i].size(); j++)
			{
				point.cartesian_x = filtered_points[i][j].cartesian_x;
				point.cartesian_y = filtered_points[i][j].cartesian_y;
				point.cartesian_z = filtered_points[i][j].cartesian_z;
			}
		}
		objects_positions.push_back(point);
	}
	return (objects_positions);
}


void
carmen_translte_2d(double *x, double *y, double offset_x, double offset_y)
{
	*x += offset_x;
	*y += offset_y;
}


void
carmen_spherical_to_cartesian(double *x, double *y, double *z, double horizontal_angle, double vertical_vangle, double range)
{
	*x = range * cos(vertical_vangle) * cos(horizontal_angle);
	*y = range * cos(vertical_vangle) * sin(horizontal_angle);
	*z = range * sin(vertical_vangle);
}


int
compute_num_measured_objects(vector<image_cartesian> objects_poses)
{
	int num_objects = 0;

	for (int i = 0; i < objects_poses.size(); i++)
	{
		if (objects_poses[i].cartesian_x > 0.0 || objects_poses[i].cartesian_y > 0.0)
			num_objects++;
	}
	return (num_objects);
}


carmen_moving_objects_point_clouds_message
build_detected_objects_message(vector<bbox_t> predictions, vector<image_cartesian> objects_poses, vector<vector<image_cartesian>> points_lists)
{
	carmen_moving_objects_point_clouds_message msg;
	int num_objects = compute_num_measured_objects(objects_poses);

	//printf ("Predictions %d Poses %d, Points %d\n", (int) predictions.size(), (int) objects_poses.size(), (int) points_lists.size());

	msg.num_point_clouds = num_objects;
	msg.point_clouds = (t_point_cloud_struct *) malloc (num_objects * sizeof(t_point_cloud_struct));

	carmen_vector_3D_t offset; // TODO ler do carmen ini
	offset.x = 0.572;
	offset.y = 0.0;
	offset.z = 2.154;

	for (int i = 0, l = 0; i < objects_poses.size(); i++)
	{
		if (objects_poses[i].cartesian_x != -999.0 || objects_poses[i].cartesian_y != -999.0)
		{
		carmen_translte_2d(&objects_poses[i].cartesian_x, &objects_poses[i].cartesian_y, offset.x, offset.y);
		carmen_rotate_2d  (&objects_poses[i].cartesian_x, &objects_poses[i].cartesian_y, globalpos.theta);
		carmen_translte_2d(&objects_poses[i].cartesian_x, &objects_poses[i].cartesian_y, globalpos.x, globalpos.y);

		msg.point_clouds[l].r = 1.0;
		msg.point_clouds[l].g = 1.0;
		msg.point_clouds[l].b = 0.0;

		msg.point_clouds[l].linear_velocity = 0;
		msg.point_clouds[l].orientation = globalpos.theta;

		msg.point_clouds[l].length = 4.5;
		msg.point_clouds[l].height = 1.8;
		msg.point_clouds[l].width  = 1.6;

		msg.point_clouds[l].object_pose.x = objects_poses[i].cartesian_x;
		msg.point_clouds[l].object_pose.y = objects_poses[i].cartesian_y;
		msg.point_clouds[l].object_pose.z = 0.0;

		switch (predictions[i].obj_id)
		{
			case 0:
				msg.point_clouds[l].geometric_model = 0;
				msg.point_clouds[l].model_features.geometry.height = 1.8;
				msg.point_clouds[l].model_features.geometry.length = 1.0;
				msg.point_clouds[l].model_features.geometry.width = 1.0;
				msg.point_clouds[l].model_features.red = 1.0;
				msg.point_clouds[l].model_features.green = 1.0;
				msg.point_clouds[l].model_features.blue = 0.8;
				msg.point_clouds[l].model_features.model_name = (char *) "pedestrian";
				break;
			case 2:
				msg.point_clouds[l].geometric_model = 0;
				msg.point_clouds[l].model_features.geometry.height = 1.8;
				msg.point_clouds[l].model_features.geometry.length = 4.5;
				msg.point_clouds[l].model_features.geometry.width = 1.6;
				msg.point_clouds[l].model_features.red = 1.0;
				msg.point_clouds[l].model_features.green = 0.0;
				msg.point_clouds[l].model_features.blue = 0.8;
				msg.point_clouds[l].model_features.model_name = (char *) "car";
				break;
			case 9:
				printf("Traffic Light %lf, %lf", objects_poses[i].cartesian_x, objects_poses[i].cartesian_y);
				break;
			default:
				msg.point_clouds[l].geometric_model = 0;
				msg.point_clouds[l].model_features.geometry.height = 1.8;
				msg.point_clouds[l].model_features.geometry.length = 4.5;
				msg.point_clouds[l].model_features.geometry.width = 1.6;
				msg.point_clouds[l].model_features.red = 1.0;
				msg.point_clouds[l].model_features.green = 1.0;
				msg.point_clouds[l].model_features.blue = 0.0;
				msg.point_clouds[l].model_features.model_name = (char *) "other";
				break;
		}
		msg.point_clouds[l].num_associated = 0;

		msg.point_clouds[l].point_size = points_lists[i].size();

		msg.point_clouds[l].points = (carmen_vector_3D_t *) malloc (msg.point_clouds[l].point_size * sizeof(carmen_vector_3D_t));

		for (int j = 0; j < points_lists[i].size(); j++)
		{
			carmen_vector_3D_t p;

			p.x = points_lists[i][j].cartesian_x;
			p.y = points_lists[i][j].cartesian_y;
			p.z = points_lists[i][j].cartesian_z;

			carmen_translte_2d(&p.x, &p.y, offset.x, offset.y);

			carmen_rotate_2d(&p.x, &p.y, globalpos.theta);

			carmen_translte_2d(&p.x, &p.y, globalpos.x, globalpos.y);

			msg.point_clouds[l].points[j] = p;
		}
		l++;
	}
	}
	return (msg);
}


vector<bbox_t>
filter_predictions_of_interest(vector<bbox_t> &predictions)
{
	vector<bbox_t> filtered_predictions;

	for (unsigned int i = 0; i < predictions.size(); i++)
	{
		if (i == 0 ||  // person
			i == 1 ||  // bicycle
			i == 2 ||  // car
			i == 3 ||  // motorbike
			i == 5 ||  // bus
			i == 6 ||  // train
			i == 7 ||  // truck
			i == 9)    // traffic light
		{
			filtered_predictions.push_back(predictions[i]);
		}
	}
	return (filtered_predictions);
}


void
compute_annotation_specifications(vector<vector<image_cartesian>> traffic_light_clusters)
{
	double mean_x = 0.0, mean_y = 0.0, mean_z = 0.0;

	for (int i = 0; i < traffic_light_clusters.size(); i++)
	{
			for (int j = 0; j < traffic_light_clusters[i].size(); j++)
			{
				mean_x += traffic_light_clusters[i][j].cartesian_x;
				mean_y += traffic_light_clusters[i][j].cartesian_y;
				mean_z += traffic_light_clusters[i][j].cartesian_z;
			}
			printf("%lf, %lf, %lf", mean_x, mean_y, mean_z);
	}
}


void
generate_traffic_light_annotations(vector<bbox_t> predictions, vector<vector<image_cartesian>> points_inside_bbox)
{
	static vector<image_cartesian> traffic_light_points;
	static int count = 0;
	int traffic_light_found = 1;

	for (int i = 0; i < predictions.size(); i++)
	{
		if (predictions[i].obj_id == 9)
		{
			printf("%s\n", obj_names_vector[predictions[i].obj_id].c_str());
			for (int j = 0; j < points_inside_bbox[i].size(); j++)
			{
				traffic_light_points.push_back(points_inside_bbox[i][j]);
			}
			traffic_light_found = 0;
		}
	}
	count += traffic_light_found;

	if (count >= 20)                // If stays without see a traffic light for more than 20 frames
	{                              // Compute traffic light positions and generate annotations
		vector<vector<image_cartesian>> traffic_light_clusters = dbscan_compute_clusters(0.5, 3, traffic_light_points);
		compute_annotation_specifications(traffic_light_clusters);
		traffic_light_points.clear();
		count = 0;
	}
}


void
image_handler(carmen_bumblebee_basic_stereoimage_message *image_msg)
{
	unsigned char *img;
	double fps;
	static double start_time = 0.0;

//	int crop_x = image_msg->width * 0.2;
//	int crop_y = image_msg->height * 0.07;
//	int crop_w = image_msg->width * 0.55;
//	int crop_h = image_msg->height * 0.5;

	int crop_x = 0;
	int crop_y = 0;
	int crop_w = image_msg->width;
	int crop_h = image_msg->height;

	if (camera_side == 0)
		img = image_msg->raw_left;
	else
		img = image_msg->raw_right;

	Mat open_cv_image = Mat(image_msg->height, image_msg->width, CV_8UC3, img, 0);              // CV_32FC3 float 32 bit 3 channels (to char image use CV_8UC3)
	//printf("%d %d\n", image_msg->width, image_msg->height);
	//printf("%d %d %d %d\n", crop_x, crop_y, crop_w, crop_h);

	Rect myROI(crop_x, crop_y, crop_w, crop_h);     // TODO put this in the .ini file
	open_cv_image = open_cv_image(myROI);

	cvtColor(open_cv_image, open_cv_image, COLOR_RGB2BGR);

	//imshow("NOD", open_cv_image);
	//waitKey(1);

	vector<bbox_t> predictions = darknet->detect(open_cv_image, 0.2);  // Arguments (image, threshold)
	predictions = filter_predictions_of_interest(predictions);

	if (predictions.size() > 0 && velodyne_msg != NULL)
	{
		//vector<image_cartesian> points = velodyne_camera_calibration_fuse_camera_lidar(velodyne_msg, camera_parameters, velodyne_pose, camera_pose,
		//		image_msg->width, image_msg->height, 280, 70, 720, 480);

		vector<image_cartesian> points = velodyne_camera_calibration_fuse_camera_lidar(velodyne_msg, camera_parameters, velodyne_pose, camera_pose,
						image_msg->width, image_msg->height, crop_x, crop_y, crop_w, crop_h);

		vector<vector<image_cartesian>> points_inside_bbox = filter_points_inside_bounding_boxes(predictions, points);

		vector<vector<image_cartesian>> filtered_points = filter_object_points_using_dbscan(points_inside_bbox);

		vector<image_cartesian> positions = compute_detected_objects_poses(filtered_points);

		generate_traffic_light_annotations(predictions, points_inside_bbox);

		//carmen_moving_objects_point_clouds_message msg = build_detected_objects_message(predictions, positions, filtered_points);

		//publish_moving_objects_message(image_msg->timestamp, &msg);

		fps = 1.0 / (carmen_get_time() - start_time);
		start_time = carmen_get_time();

		//printf("%d %d %d\n", (int) predictions.size(), (int) points_inside_bbox.size(), (int) filtered_points.size());

		show_detections(open_cv_image, predictions, points, points_inside_bbox, filtered_points, fps, crop_x, crop_y, crop_w, crop_h);
	}
}


void
velodyne_partial_scan_message_handler(carmen_velodyne_partial_scan_message *velodyne_message)
{
	velodyne_msg = velodyne_message;

    carmen_velodyne_camera_calibration_arrange_velodyne_vertical_angles_to_true_position(velodyne_msg);
}


void
localize_ackerman_globalpos_message_handler(carmen_localize_ackerman_globalpos_message *globalpos_message)
{
	globalpos.theta = globalpos_message->globalpos.theta;
	globalpos.x = globalpos_message->globalpos.x;
	globalpos.y = globalpos_message->globalpos.y;
}


void
shutdown_module(int signo)
{
    if (signo == SIGINT) {
        carmen_ipc_disconnect();
        cvDestroyAllWindows();

        printf("Neural Object Detector: Disconnected.\n");
        exit(0);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////


void
subscribe_messages()
{
    carmen_bumblebee_basic_subscribe_stereoimage(camera, NULL, (carmen_handler_t) image_handler, CARMEN_SUBSCRIBE_LATEST);

    carmen_velodyne_subscribe_partial_scan_message(NULL, (carmen_handler_t) velodyne_partial_scan_message_handler, CARMEN_SUBSCRIBE_LATEST);

    carmen_localize_ackerman_subscribe_globalpos_message(NULL, (carmen_handler_t) localize_ackerman_globalpos_message_handler, CARMEN_SUBSCRIBE_LATEST);
}


void
read_parameters(int argc, char **argv)
{
	if ((argc != 3))
		carmen_die("%s: Wrong number of parameters. neural_object_detector requires 2 parameter and received %d. \n Usage: %s <camera_number> <camera_side(0-left; 1-right)\n>", argv[0], argc - 1, argv[0]);

	camera = atoi(argv[1]);             // Define the camera to be used
    camera_side = atoi(argv[2]);        // 0 For left image 1 for right image

    int num_items;

    char bumblebee_string[256];
    char camera_string[256];

    sprintf(bumblebee_string, "%s%d", "bumblebee_basic", camera); // Geather the cameri ID
    sprintf(camera_string, "%s%d", "camera", camera);

    carmen_param_t param_list[] =
    {
		{bumblebee_string, (char*) "fx", CARMEN_PARAM_DOUBLE, &camera_parameters.fx_factor, 0, NULL },
		{bumblebee_string, (char*) "fy", CARMEN_PARAM_DOUBLE, &camera_parameters.fy_factor, 0, NULL },
		{bumblebee_string, (char*) "cu", CARMEN_PARAM_DOUBLE, &camera_parameters.cu_factor, 0, NULL },
		{bumblebee_string, (char*) "cv", CARMEN_PARAM_DOUBLE, &camera_parameters.cv_factor, 0, NULL },
		{bumblebee_string, (char*) "pixel_size", CARMEN_PARAM_DOUBLE, &camera_parameters.pixel_size, 0, NULL },

		{(char *) "velodyne", (char *) "x",     CARMEN_PARAM_DOUBLE, &(velodyne_pose.position.x), 0, NULL},
		{(char *) "velodyne", (char *) "y",     CARMEN_PARAM_DOUBLE, &(velodyne_pose.position.y), 0, NULL},
		{(char *) "velodyne", (char *) "z",     CARMEN_PARAM_DOUBLE, &(velodyne_pose.position.z), 0, NULL},
		{(char *) "velodyne", (char *) "roll",  CARMEN_PARAM_DOUBLE, &(velodyne_pose.orientation.roll), 0, NULL},
		{(char *) "velodyne", (char *) "pitch", CARMEN_PARAM_DOUBLE, &(velodyne_pose.orientation.pitch), 0, NULL},
		{(char *) "velodyne", (char *) "yaw",   CARMEN_PARAM_DOUBLE, &(velodyne_pose.orientation.yaw), 0, NULL},

		{camera_string, (char*) "x",     CARMEN_PARAM_DOUBLE, &camera_pose.position.x, 0, NULL },
		{camera_string, (char*) "y",     CARMEN_PARAM_DOUBLE, &camera_pose.position.y, 0, NULL },
		{camera_string, (char*) "z",     CARMEN_PARAM_DOUBLE, &camera_pose.position.z, 0, NULL },
		{camera_string, (char*) "roll",  CARMEN_PARAM_DOUBLE, &camera_pose.orientation.roll, 0, NULL },
		{camera_string, (char*) "pitch", CARMEN_PARAM_DOUBLE, &camera_pose.orientation.pitch, 0, NULL },
		{camera_string, (char*) "yaw",   CARMEN_PARAM_DOUBLE, &camera_pose.orientation.yaw, 0, NULL }
    };

    num_items = sizeof(param_list) / sizeof(param_list[0]);
    carmen_param_install_params(argc, argv, param_list, num_items);
}


void
initialize_yolo_DRKNET()
{
	string darknet_home = getenv("DARKNET_HOME");  // Get environment variable pointing path of DARKNET_HOME

	if (darknet_home.empty())
		printf("Cannot find darknet path. Check if you have correctly set DARKNET_HOME environment variable.\n");

	string cfg_filename = darknet_home + "/cfg/neural_object_detector_yolo.cfg";
	string weight_filename = darknet_home + "/yolo.weights";
	string class_names_file = darknet_home + "/data/coco.names";

	darknet = new Detector(cfg_filename, weight_filename, 0);   // (cfg_filename, weight_filename, GPU ID)

	carmen_test_alloc(darknet);

	objects_names_from_file(class_names_file);

	set_object_vector_color();

	velodyne_msg = NULL;

	//namedWindow( "Display window", WINDOW_AUTOSIZE);
}


int
main(int argc, char **argv)
{
    carmen_ipc_initialize(argc, argv);

    read_parameters(argc, argv);

    subscribe_messages();

    signal(SIGINT, shutdown_module);

    initialize_yolo_DRKNET();

	setlocale(LC_ALL, "C");

    carmen_ipc_dispatch();

    return 0;
}
