#include <carmen/carmen.h>
#include <carmen/grid_mapping.h>
#include <carmen/road_mapper.h>

#include <wordexp.h>
#include "road_mapper_utils.h"
#include <vector>
using namespace std;

static carmen_point_t g_global_pos;
static int g_sample_width = 0;
static int g_sample_height = 0;
static double g_distance_samples = 0.0;
static int g_n_offsets = 0;
static int g_n_rotations = 0;
static double g_distance_offset = 0.0;
wordexp_t g_out_path_p;
char* g_out_path;
static int g_image_channels = 0;
static int g_image_class_bits = 0;
static int g_remission_image_channels = 0;
int g_verbose = 0;

cv::Mat *g_road_map_img;
cv::Mat *g_road_map_img3;
cv::Mat *g_remission_map_img;
cv::Mat *g_remission_map_img3;

#define VEC_SIZE 20
static carmen_map_p g_vec_remission_map[VEC_SIZE];
static carmen_map_p g_vec_road_map[VEC_SIZE];
static vector<carmen_point_t> g_vec_pos;

void
generate_sample(cv::Mat map_img, cv::Point center, double angle, cv::Rect roi, char* path)
{
	cv::Mat rot_img = rotate(map_img, center, angle);
	cv::Mat sample = rot_img(roi);
	cv::imwrite(path, sample);
	rot_img.release();
	sample.release();
}

void
generate_rotate_samples(int x, int y,
						double offset_i_meters,
						char* dir_name)
{
	double delta_rotation = 360.0 / g_n_rotations;
	int x_sample_origin = x - g_sample_width / 2;
	int y_sample_origin = y - g_sample_height / 2;
	char name[256];
	char path[2048];
	cv::Point pt = cv::Point(x, y);
	// ROI point is on the top-left corner
	cv::Rect roi = cv::Rect(cv::Point(x_sample_origin, y_sample_origin),
					cv::Size(g_sample_width, g_sample_height));
	for (int i = 0; i < g_n_rotations; i++)
	{
		double rotation_angle = i * delta_rotation;
		sprintf(name, "i%.0lf_%.0lf_%.2lf_%.2lf.png",
										g_global_pos.x, g_global_pos.y,
										offset_i_meters, rotation_angle);
		if (g_remission_image_channels == 1 || g_remission_image_channels == '*')
		{
			sprintf(path, "%s/%s/%c%s", g_out_path, dir_name, 'h', &name[1]);
			generate_sample(*g_remission_map_img, pt, rotation_angle, roi, path);
		}
		if (g_remission_image_channels == 3 || g_remission_image_channels == '*')
		{
			sprintf(path, "%s/%s/%c%s", g_out_path, dir_name, 'i', &name[1]);
			generate_sample(*g_remission_map_img3, pt, rotation_angle, roi, path);
		}
		if (g_image_channels == 1 || g_image_channels == '*')
		{
			sprintf(path, "%s/%s/%c%s", g_out_path, dir_name, 'r', &name[1]);
			generate_sample(*g_road_map_img, pt, rotation_angle, roi, path);
		}
		if (g_image_channels == 3 || g_image_channels == '*')
		{
			sprintf(path, "%s/%s/%c%s", g_out_path, dir_name, 't', &name[1]);
			generate_sample(*g_road_map_img3, pt, rotation_angle, roi, path);
		}
	}
}

void
generate_offset_samples(int x, int y, double resolution)
{
	int i;
	int offset_i_x = 0;
	int offset_i_y = 0;
	double offset_i = 0;
	double offset_i_meters = 0;
	double distance_offset = g_distance_offset / resolution;
	int x_sample_center = 0;
	int y_sample_center = 0;
	char dir_name[256];
	char command[1024];
	sprintf(dir_name, "%.0lf_%.0lf", g_global_pos.x, g_global_pos.y);
	sprintf(command, "mkdir %s/%s/", g_out_path, dir_name);
	system(command);
	for (i = -g_n_offsets; i <= g_n_offsets; i++)
	{
		offset_i_meters = i * g_distance_offset;
		offset_i = i * distance_offset;
		// OpenCV Y-axis is downward, but theta Y-axis is upward. So we change the sign of theta.
		offset_i_x = offset_i * sin(-g_global_pos.theta);
		offset_i_y = -offset_i * cos(-g_global_pos.theta);

		x_sample_center = x + offset_i_x;
		y_sample_center = y + offset_i_y;

		generate_rotate_samples(x_sample_center,
								y_sample_center,
								offset_i_meters,
								dir_name);
	}
}

int
generate_samples(void)
{
	int i_remission_map = global_pos_on_map_q4(g_global_pos, g_vec_remission_map, VEC_SIZE);
	int i_road_map = global_pos_on_map_q4(g_global_pos, g_vec_road_map, VEC_SIZE);
	if (i_remission_map == -1 || i_road_map == -1)
	{
		printf("globalpos %.2lf %.2lf: Could not find remission_map[%d] or road_map[%d]\n", g_global_pos.x, g_global_pos.y, i_remission_map, i_road_map);
		return -1;
	}

	carmen_map_p remission_map = g_vec_remission_map[i_remission_map];
	carmen_map_p road_map = g_vec_road_map[i_road_map];
	if (!maps_has_same_origin(remission_map, road_map))
	{
		printf("Sync remission_map[%d] and road_map[%d] do not have the same origin!\n", i_remission_map, i_road_map);
		return -1;
	}

	int x_map = round((g_global_pos.x - remission_map->config.x_origin) / remission_map->config.resolution);
	int y_map = round((g_global_pos.y - remission_map->config.y_origin) / remission_map->config.resolution);
	int x_img = x_map;
	int y_img = remission_map->config.y_size - 1 - y_map; // // OpenCV Y-axis is downward, but CARMEN Y-axis is upward

	if (g_remission_image_channels == 1 || g_remission_image_channels == '*')
	{
		remission_map_to_image(remission_map, g_remission_map_img, 1);
	}
	if (g_remission_image_channels == 3 || g_remission_image_channels == '*')
	{
		remission_map_to_image(remission_map, g_remission_map_img3, 3);
	}
	if (g_image_channels == 1 || g_image_channels == '*')
	{
		road_map_to_image_black_and_white(road_map, g_road_map_img, g_image_class_bits);
	}
	if (g_image_channels == 3 || g_image_channels == '*')
	{
		road_map_to_image(road_map, g_road_map_img3);
	}
	//if (g_remission_image_channels == 1)	g_remission_map_img->at<uchar>(cv::Point(x_img, y_img)) = 255;
	//if (g_remission_image_channels == 3)	g_remission_map_img3->at<cv::Vec3b>(cv::Point(x_img, y_img)) = cv::Vec3b(255, 255, 255);
	//if (g_image_channels == 1)			g_road_map_img->at<uchar>(cv::Point(x_img, y_img)) = 0;
	//if (g_image_channels == 3)			g_road_map_img3->at<cv::Vec3b>(cv::Point(x_img, y_img)) = cv::Vec3b(0, 0, 0);
	generate_offset_samples(x_img, y_img, remission_map->config.resolution);
	return 0;
}

static void
read_parameters(int argc, char **argv)
{
	char *out_path = (char *)".";
	char **w;
	char *image_channels = (char *)"*";
	char *remission_image_channels = (char *)"*";
	carmen_param_t param_list[] =
	{
			{(char*)"road_mapper",  (char*)"sample_width", 				CARMEN_PARAM_INT, 		&(g_sample_width), 				0, NULL},
			{(char*)"road_mapper",  (char*)"sample_height",				CARMEN_PARAM_INT, 		&(g_sample_height), 			0, NULL},
			{(char*)"road_mapper",  (char*)"distance_sample",			CARMEN_PARAM_DOUBLE, 	&(g_distance_samples), 			0, NULL},
			{(char*)"road_mapper",  (char*)"n_offset",					CARMEN_PARAM_INT, 		&(g_n_offsets), 				0, NULL},
			{(char*)"road_mapper",  (char*)"n_rotation",				CARMEN_PARAM_INT, 		&(g_n_rotations), 				0, NULL},
			{(char*)"road_mapper",  (char*)"distance_offset",			CARMEN_PARAM_DOUBLE, 	&(g_distance_offset),			0, NULL},
			{(char*)"road_mapper",  (char*)"out_path",					CARMEN_PARAM_STRING, 	&(out_path),					0, NULL},
			{(char*)"road_mapper",  (char*)"image_channels",			CARMEN_PARAM_STRING, 	&(image_channels),				0, NULL},
			{(char*)"road_mapper",  (char*)"image_class_bits",			CARMEN_PARAM_INT, 		&(g_image_class_bits),			0, NULL},
			{(char*)"road_mapper",  (char*)"remission_image_channels",	CARMEN_PARAM_STRING, 	&(remission_image_channels),	0, NULL},
	};

	carmen_param_install_params(argc, argv, param_list, sizeof(param_list) / sizeof(param_list[0]));

	// expand environment variables on path to full path
	wordexp(out_path, &g_out_path_p, 0 );
	w = g_out_path_p.we_wordv;
	g_out_path = *w;

	// image channels
	g_image_channels = '*';
	if(strcmp(image_channels, "1") == 0 || strcmp(image_channels, "3") == 0)
	{
		g_image_channels = atoi(image_channels);
	}
	g_remission_image_channels = '*';
	if(strcmp(remission_image_channels, "1") == 0 || strcmp(remission_image_channels, "3") == 0)
	{
		g_remission_image_channels = atoi(remission_image_channels);
	}

	const char usage[] = "[-v]";
	for(int i = 1; i < argc; i++)
	{
		if(strncmp(argv[i], "-h", 2) == 0 || strncmp(argv[i], "--help", 6) == 0)
		{
			printf("Usage:\n%s %s\n", argv[0], usage);
			exit(1);
		}
		else if(strncmp(argv[i], "-v", 2) == 0 || strncmp(argv[i], "--verbose", 9) == 0)
		{
			g_verbose = 1;
			printf("Verbose option set.\n");
		}
		else
		{
			printf("Ignored command line parameter: %s\n", argv[i]);
			printf("Usage:\n%s %s\n", argv[0], usage);
		}
	}
}

static void
define_messages()
{
	carmen_localize_ackerman_define_globalpos_messages();
	carmen_map_server_define_localize_map_message();
	carmen_map_server_define_road_map_message();
}

static void
generate_all(void)
{
	vector<carmen_point_t>::iterator pos = g_vec_pos.begin();

	while (pos != g_vec_pos.end())
	{
		memcpy(&g_global_pos, &(*pos), sizeof(carmen_point_t));
		int generate_err = generate_samples();
		if (generate_err == 0)
		{
			pos = g_vec_pos.erase(pos);
		}
		else
		{
			pos++;
		}
	}
	if (g_verbose && !g_vec_pos.empty())
	{
		printf("%ld poses still queued...\n", g_vec_pos.size());
	}
	fflush(stdout);
}

static void
global_pos_handler(carmen_localize_ackerman_globalpos_message *msg)
{
	static int first_time = 1;
	static carmen_point_t previous_global_pos;
	static int count_pos = 0, count_push = 0;
	int good_to_push;
	char status1[80] = "", status2[80] = "", status3[80] = "";

	sprintf(status1, "localize_ackerman globalpos [%d]: %.2lf %.2lf", ++count_pos, msg->globalpos.x, msg->globalpos.y);
	if (first_time)
	{
		good_to_push = 1;
		first_time = 0;
	}
	else
	{
		double distance = carmen_distance(&previous_global_pos, &msg->globalpos);
		good_to_push = (distance >= g_distance_samples);
		sprintf(status2, "distance %.2lf previous %.2lf %.2lf", distance, previous_global_pos.x, previous_global_pos.y);
	}
	if (good_to_push)
	{
		memcpy(&previous_global_pos, &msg->globalpos, sizeof(carmen_point_t));
		g_vec_pos.push_back(msg->globalpos);
		sprintf(status3, "PUSHED [%d]", ++count_push);
	}
	if (g_verbose)
	{
		printf("%s %s %s\n", status1, status2, status3);
	}
	generate_all();
}

static void
localize_map_handler(carmen_map_server_localize_map_message *msg)
{
	static int first_time = 1;
	static int count = 0;
	if (first_time == 1)
	{
		int i;
		for (i = 0; i < VEC_SIZE; i++)
		{
			carmen_grid_mapping_initialize_map(g_vec_remission_map[i],
												msg->config.x_size,
												msg->config.resolution, 'm');
		}
		if (g_remission_image_channels == 1 || g_remission_image_channels == '*')
		{
			g_remission_map_img = new cv::Mat(g_vec_remission_map[0]->config.y_size,
												g_vec_remission_map[0]->config.x_size,
												CV_8UC1);
		}
		if (g_remission_image_channels == 3 || g_remission_image_channels == '*')
		{
			g_remission_map_img3 = new cv::Mat(g_vec_remission_map[0]->config.y_size,
												g_vec_remission_map[0]->config.x_size,
												CV_8UC3,
												cv::Scalar::all(0));
		}
		first_time = 0;
	}
	memcpy(g_vec_remission_map[count % VEC_SIZE]->complete_map,
			msg->complete_mean_remission_map,
			sizeof(double) * msg->size);
	memcpy(&g_vec_remission_map[count % VEC_SIZE]->config,
			&msg->config,
			sizeof(carmen_map_config_t));
	count++;
	if (g_verbose)
	{
		printf("map_server remission [%d]: %.2lf %.2lf %dx%d\n", count, msg->config.x_origin, msg->config.y_origin, msg->config.x_size, msg->config.y_size);
	}
	generate_all();
	if (g_verbose && g_vec_pos.empty())
	{
		printf("Pose queue is empty.\n");
	}
	fflush(stdout);
}

static void
road_map_handler(carmen_map_server_road_map_message *msg)
{
	static int first_time = 1;
	static int count = 0;
	if (first_time == 1)
	{
		int i;
		for (i = 0; i < VEC_SIZE; i++)
		{
			carmen_grid_mapping_initialize_map(g_vec_road_map[i],
												msg->config.x_size,
												msg->config.resolution, 'r');
		}
		if (g_image_channels == 1 || g_image_channels == '*')
		{
			g_road_map_img = new cv::Mat(g_vec_road_map[0]->config.y_size,
										g_vec_road_map[0]->config.x_size,
										CV_8UC1);
		}
		if (g_image_channels == 3 || g_image_channels == '*')
		{
			g_road_map_img3 = new cv::Mat(g_vec_road_map[0]->config.y_size,
										g_vec_road_map[0]->config.x_size,
										CV_8UC3,
										cv::Scalar::all(0));
		}
		first_time = 0;
	}
	memcpy(g_vec_road_map[count % VEC_SIZE]->complete_map,
			msg->complete_map,
			sizeof(double) * msg->size);
	memcpy(&g_vec_road_map[count % VEC_SIZE]->config,
			&msg->config,
			sizeof(carmen_map_config_t));
	count++;
	if (g_verbose)
	{
		printf("map_server road_map  [%d]: %.2lf %.2lf %dx%d\n", count, msg->config.x_origin, msg->config.y_origin, msg->config.x_size, msg->config.y_size);
	}
	generate_all();
	if (g_verbose && g_vec_pos.empty())
	{
		printf("Pose queue is empty.\n");
	}
	fflush(stdout);
}

static void
register_handlers()
{
	carmen_localize_ackerman_subscribe_globalpos_message(NULL,
															(carmen_handler_t) global_pos_handler,
															CARMEN_SUBSCRIBE_ALL);

	carmen_map_server_subscribe_localize_map_message(NULL,
														(carmen_handler_t) localize_map_handler,
														CARMEN_SUBSCRIBE_ALL);

	carmen_map_server_subscribe_road_map(NULL,
												(carmen_handler_t) road_map_handler,
												CARMEN_SUBSCRIBE_ALL);
}

static void
initialize_maps(void)
{
	int i;
	for (i = 0; i < VEC_SIZE; i++)
	{
		g_vec_remission_map[i] = alloc_map_pointer();
		g_vec_road_map[i] = alloc_map_pointer();
	}
}

void
deinitialize_maps(void)
{
	int i;
	for (i = 0; i < VEC_SIZE; i++)
	{
		free_map_pointer(g_vec_remission_map[i]);
		free_map_pointer(g_vec_road_map[i]);
	}

	g_remission_map_img->release();
	g_remission_map_img3->release();
	g_road_map_img->release();
	g_road_map_img3->release();

	wordfree(&g_out_path_p);
}

void
shutdown_module(int signo)
{
	if (signo == SIGINT)
	{
		carmen_ipc_disconnect();
		printf("road_mapper_sampling: disconnected.\n");
		deinitialize_maps();
		exit(0);
	}
}

int
main(int argc, char **argv)
{
	carmen_ipc_initialize(argc, argv);
	carmen_param_check_version(argv[0]);
	read_parameters(argc, argv);
	define_messages();

	signal(SIGINT, shutdown_module);

	initialize_maps();

	register_handlers();
	carmen_ipc_dispatch();

	return 0;
}
