
#include <string>
#include <carmen/command_line.h>
#include <carmen/segmap_preproc.h>
#include <carmen/segmap_grid_map.h>
#include <carmen/segmap_particle_filter.h>
#include <carmen/segmap_dataset.h>
#include <carmen/segmap_conversions.h>
#include <carmen/segmap_constructors.h>

using namespace std;


SensorPreproc::IntensityMode
parse_intensity_mode(string map_type)
{
	if (map_type.compare("remission") == 0)
		return SensorPreproc::INTENSITY;
	else if (map_type.compare("visual") == 0)
		return SensorPreproc::COLOR;
	else if (map_type.compare("semantic") == 0)
		return SensorPreproc::SEMANTIC;
	else
		exit(printf("Error: invalid map type '%s'.\n", map_type.c_str()));
}


GridMap
create_grid_map(CommandLineArguments &args)
{
	int map_type;
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
							map_type, 0);

	return map;
}


ParticleFilter
create_particle_filter(CommandLineArguments &args)
{
	ParticleFilter::WeightType wtype;

	if (args.get<int>("use_gps_weight"))
		wtype = ParticleFilter::WEIGHT_GPS;
	else if (args.get<string>("intensity_mode").compare("semantic") == 0)
		wtype = ParticleFilter::WEIGHT_SEMANTIC;
	else
		wtype = ParticleFilter::WEIGHT_VISUAL;

	ParticleFilter pf(args.get<int>("n_particles"),
										wtype,
										args.get<double>("gps_xy_std"),
										args.get<double>("gps_xy_std"),
										degrees_to_radians(args.get<double>("gps_h_std")),
										args.get<double>("v_std"),
										degrees_to_radians(args.get<double>("phi_std")),
										args.get<double>("odom_xy_std"),
										args.get<double>("odom_xy_std"),
										degrees_to_radians(args.get<double>("odom_h_std")),
										args.get<double>("color_red_std"),
										args.get<double>("color_green_std"),
										args.get<double>("color_blue_std"));

	return pf;
}


NewCarmenDataset*
create_dataset(string log_path)
{
	string odom_calib_path = default_odom_calib_path(log_path.c_str());
	string fused_odom_path = default_fused_odom_path(log_path.c_str());
	string graphslam_path = default_graphslam_path(log_path.c_str());

	NewCarmenDataset *dataset = new NewCarmenDataset(log_path, odom_calib_path, graphslam_path);

	return dataset;
}


SensorPreproc
create_sensor_preproc(CommandLineArguments &args,
											NewCarmenDataset *dataset,
											string log_path)
{
	CarmenLidarLoader *vloader = new CarmenLidarLoader;
	CarmenImageLoader *iloader = new CarmenImageLoader;
	SemanticSegmentationLoader *sloader = new SemanticSegmentationLoader(log_path);

	SensorPreproc::IntensityMode i_mode;
	i_mode = parse_intensity_mode(args.get<string>("intensity_mode"));

	SensorPreproc preproc(vloader, iloader, sloader,
												dataset->vel2cam(), dataset->vel2car(), dataset->projection_matrix(),
												dataset->xsens2car(), args.get<int>("use_xsens"), dataset->at(0)->pose,
												i_mode,
												args.get<double>("ignore_above_threshold"),
												args.get<double>("ignore_below_threshold"));

	return preproc;
}
