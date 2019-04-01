
#include <carmen/segmap_dataset.h>
#include <carmen/segmap_grid_map.h>
#include <carmen/segmap_sensor_viewer.h>
#include <carmen/segmap_particle_filter_viewer.h>
#include <carmen/segmap_semantic_segmentation_viewer.h>
#include <carmen/segmap_preproc.h>
#include <carmen/util_time.h>
#include <carmen/util_math.h>
#include <carmen/command_line.h>
#include "libsegmap/initializations/segmap_args.h"
#include <carmen/segmap_constructors.h>

using namespace cv;
using namespace std;
using namespace pcl;


void
update_map(DataSample *sample, GridMap *map, SensorPreproc &preproc)
{
	preproc.reinitialize(sample);

	for (int i = 0; i < preproc.size(); i++)
	{
		vector<PointXYZRGB> points = preproc.next_points_in_world();

		for (int j = 0; j < points.size(); j++)
			map->add_point(points[j]);
	}
}


void
view(GridMap &map, DataSample *sample, Pose2d &offset, PointCloudViewer &viewer)
{
	Pose2d pose;
	pose = sample->pose;

	pose.x -= offset.x;
	pose.y -= offset.y;

	Mat map_img = map.to_image().clone();
	draw_pose(map, map_img, pose, Scalar(0, 255, 0));

	// flip vertically.
	Mat map_view;
	flip(map_img, map_view, 0);

	//viewer.clear();
	//viewer.show(colored);
	//viewer.show(img, "img", 640);
	//viewer.show(simg, "simg", 640);
	//viewer.show(simg_view, "simg_view", 640);
	viewer.show(map_view, "map", 640);
	viewer.loop();
}


void
create_map(GridMap &map, NewCarmenDataset *dataset, int step,
					 SensorPreproc &preproc, Pose2d &offset, double skip_velocity_threshold)
{
	TimeCounter timer;
	DataSample *sample;
	PointCloudViewer viewer;
	vector<double> times;

	for (int i = 0; i < dataset->size(); i += step)
	{
		sample = dataset->at(i);

		if (fabs(sample->v) < skip_velocity_threshold)
			continue;

		timer.start();

		map.reload(sample->pose.x - offset.x, sample->pose.y - offset.y);
		update_map(sample, &map, preproc);

		times.push_back(timer.ellapsed());

		if (times.size() % 50 == 0)
			printf("Avg ellapsed %ld: %lf Current: %lf\n",
						 times.size(),
						 mean(times),
						 times[times.size() - 1]);

		view(map, sample, offset, viewer);
	}
}


int
main(int argc, char **argv)
{
	int map_type;
	string log_path;

	CommandLineArguments args;
	add_default_slam_args(args);
	add_default_sensor_preproc_args(args);
	add_default_mapper_args(args);
	args.save_config_file(default_data_dir() + "/mapper_config.txt");
	args.parse(argc, argv);

	log_path = args.get<string>("log_path");

	string odom_calib_path = default_odom_calib_path(log_path.c_str());
	string fused_odom_path = default_fused_odom_path(log_path.c_str());
	string graphslam_path = default_graphslam_path(log_path.c_str());
	string graphslam_to_map_path = default_graphslam_to_map_path(log_path.c_str());

	SensorPreproc::IntensityMode i_mode;
	i_mode = parse_intensity_mode(args.get<string>("intensity_mode"));

	if (i_mode == SensorPreproc::SEMANTIC)
		map_type = GridMapTile::TYPE_SEMANTIC;
	else
		map_type = GridMapTile::TYPE_VISUAL;

	GridMap map(args.get<string>("map_path"),
							args.get<double>("tile_size"),
							args.get<double>("tile_size"),
							args.get<double>("resolution"),
							map_type, 1);

	NewCarmenDataset *dataset = new NewCarmenDataset(log_path, odom_calib_path, graphslam);

	Pose2d offset = dataset->at(0)->pose;

	SensorPreproc preproc = create_sensor_preproc(args, dataset, log_path);

	create_map(map, dataset, args.get<int>("step"), preproc, offset,
						 args.get<double>("v_thresh"));

	printf("Done.\n");
	return 0;
}

