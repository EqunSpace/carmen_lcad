#ifndef SRC_VELODYNE_CAMERA_CALIBRATION_VELODYNE_CAMERA_CALIBRATION_H_
#define SRC_VELODYNE_CAMERA_CALIBRATION_VELODYNE_CAMERA_CALIBRATION_H_

#include <carmen/carmen.h>
#include <carmen/bumblebee_basic_interface.h>
#include <carmen/velodyne_interface.h>
#include <vector>
#include <tf.h>

using namespace std;

typedef struct
{
	carmen_velodyne_points_in_cam_t velodyne_points_in_cam;
	bool hit_in_obstacle;
} carmen_velodyne_points_in_cam_with_obstacle_t, *carmen_velodyne_points_in_cam_with_obstacle_p;


typedef struct
{
	unsigned int image_x;
	unsigned int image_y;
	carmen_sphere_coord_t polar;
	carmen_vector_3D_t cartesian;
} velodyne_camera_points;


// These parameters are given in percentual form
typedef struct
{
	double fx_factor;
	double fy_factor;
	double cu_factor;
	double cv_factor;
	double pixel_size;
	double baseline;
} carmen_camera_parameters;


typedef struct
{
	int shot_number;
	int ray_number;
    int image_x;
    int image_y;
    double cartesian_x;
    double cartesian_y;
    double cartesian_z;
}image_cartesian;


void
carmen_velodyne_camera_calibration_arrange_velodyne_vertical_angles_to_true_position(carmen_velodyne_partial_scan_message *velodyne_message);

std::vector<carmen_velodyne_points_in_cam_t>
carmen_velodyne_camera_calibration_lasers_points_in_camera(carmen_velodyne_partial_scan_message *velodyne_message,
                                                           carmen_camera_parameters camera_parameters,
                                                           carmen_pose_3D_t velodyne_pose, carmen_pose_3D_t camera_pose,
                                                           int image_width, int image_height);

std::vector<carmen_velodyne_points_in_cam_t>
carmen_velodyne_camera_calibration_lasers_points_in_camera_with_obstacle(carmen_velodyne_partial_scan_message *velodyne_message,
                                                                         carmen_camera_parameters camera_parameters,
                                                                         carmen_pose_3D_t velodyne_pose, carmen_pose_3D_t camera_pose,
                                                                         int image_width, int image_height);

std::vector<carmen_velodyne_points_in_cam_with_obstacle_t>
carmen_velodyne_camera_calibration_lasers_points_in_camera_with_obstacle_and_display(carmen_velodyne_partial_scan_message *velodyne_message,
                                                                                     carmen_camera_parameters camera_parameters,
                                                                                     carmen_pose_3D_t velodyne_pose, carmen_pose_3D_t camera_pose,
                                                                                     int image_width, int image_height);

vector<image_cartesian>
velodyne_camera_calibration_fuse_camera_lidar(carmen_velodyne_partial_scan_message *velodyne_message, carmen_camera_parameters camera_parameters,
                                                                         carmen_pose_3D_t velodyne_pose, carmen_pose_3D_t camera_pose,
																		 unsigned int image_width, unsigned int image_height, unsigned int crop_x,
																		 unsigned int crop_y, unsigned int crop_width, unsigned int crop_height);

void
initialize_transformations(carmen_pose_3D_t board_pose, carmen_pose_3D_t camera_pose, tf::Transformer *transformer);


tf::StampedTransform
get_world_to_camera_transformation (tf::Transformer *transformer, carmen_pose_3D_t pose);


tf::StampedTransform
get_world_to_camera_transformer(tf::Transformer *transformer, double x, double y, double z, double roll, double pitch, double yaw);


carmen_position_t
convert_rddf_pose_to_point_in_image(double x, double y, double z, tf::StampedTransform world_to_camera_pose,
									carmen_camera_parameters camera_parameters, int image_width, int image_height);

#endif // SRC_VELODYNE_CAMERA_CALIBRATION_VELODYNE_CAMERA_CALIBRATION_H_
