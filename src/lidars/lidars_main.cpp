#include <carmen/carmen.h>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>

#include <carmen/velodyne_interface.h>
#include <driver2.h>
#include <rsdriver.h>

#include "lidar_drivers.h"

using namespace std;

velodyne_driver::velodyne_gps_t gps;
velodyne_driver::velodyne_config_t config;
velodyne_driver::VelodyneDriver* velodyne = NULL;

carmen_velodyne_variable_scan_message variable_scan_msg;
static carmen_velodyne_gps_message velodyne_gps;

//static int velodyne_scan_port;
//static int velodyne_gps_port;
static int velodyne_gps_enabled;
static int velodyne_number;
static char* velodyne_model;

static double velodyne_package_rate;
static double dist_lsb;
static double rotation_resolution;
static unsigned short rotation_max_units;
static int velodyne_num_lasers;							//vertical_resolution
static int velodyne_num_shots;
static int min_sensing_distance;
static int max_sensing_distance;
static int velodyne_msg_buffer_size;
static int velodyne_gps_buffer_size;

static double velodyne_driver_broadcast_freq_hz;
static int velodyne_udp_port; 							//velodyne_scan_port
static int velodyne_gps_udp_port;						//velodyne_gps_port

static unsigned short velodyne_upper_header_bytes;
static unsigned short velodyne_lower_header_bytes;

static double gyro_scale_factor;
static double temp_scale_factor;
static int temp_base_factor;
static double accel_scale_factor;

static double velodyne_min_frequency;
static int velodyne_max_laser_shots_per_revolution;


rslidar_driver::rslidar_param private_nh;
rslidar_driver::rslidarDriver* robosense = NULL;

void assembly_velodyne_gps_message_from_gps(velodyne_driver::velodyne_gps_t gps)
{
	velodyne_gps.gyro1 = (gps.packet.gyro1 & 0x0fff) * gyro_scale_factor;
	velodyne_gps.gyro2 = (gps.packet.gyro2 & 0x0fff) * gyro_scale_factor;
	velodyne_gps.gyro3 = (gps.packet.gyro3 & 0x0fff) * gyro_scale_factor;

	velodyne_gps.temp1 = (gps.packet.temp1 & 0x0fff) * temp_scale_factor + temp_base_factor;
	velodyne_gps.temp2 = (gps.packet.temp2 & 0x0fff) * temp_scale_factor + temp_base_factor;
	velodyne_gps.temp3 = (gps.packet.temp3 & 0x0fff) * temp_scale_factor + temp_base_factor;

	velodyne_gps.accel1_x = (gps.packet.accel1_x & 0x0fff) * accel_scale_factor;
	velodyne_gps.accel2_x = (gps.packet.accel2_x & 0x0fff) * accel_scale_factor;
	velodyne_gps.accel3_x = (gps.packet.accel3_x & 0x0fff) * accel_scale_factor;

	velodyne_gps.accel1_y = (gps.packet.accel1_y & 0x0fff) * accel_scale_factor;
	velodyne_gps.accel2_y = (gps.packet.accel2_y & 0x0fff) * accel_scale_factor;
	velodyne_gps.accel3_y = (gps.packet.accel3_y & 0x0fff) * accel_scale_factor;

	char* nmea_sentence = (char*) gps.packet.nmea_sentence;
	std::vector<std::string> strs;
	boost::split(strs, nmea_sentence, boost::is_any_of(","));

	velodyne_gps.utc_time = atoi(strs[1].c_str());
	velodyne_gps.status = *strs[2].c_str();
	velodyne_gps.latitude = (double) atof(strs[3].c_str());
	velodyne_gps.latitude_hemisphere = *strs[4].c_str();
	velodyne_gps.longitude = (double) atof(strs[5].c_str());
	velodyne_gps.longitude_hemisphere = *strs[6].c_str();
	velodyne_gps.speed_over_ground = (double) atof(strs[7].c_str());
	velodyne_gps.course_over_ground = (double) atof(strs[8].c_str());
	velodyne_gps.utc_date = atoi(strs[9].c_str());
	velodyne_gps.magnetic_variation_course = (double) atof(strs[10].c_str());
	velodyne_gps.magnetic_variation_direction = (double) atof(strs[11].c_str());
	velodyne_gps.mode_indication = *strs[12].c_str();

	velodyne_gps.timestamp = gps.timestamp;
	velodyne_gps.host = carmen_get_host();

//	printf("Gyro    -> 1: %6.2f, 2: %6.2f, 3: %6.2f\n", velodyne_gps.gyro1, velodyne_gps.gyro2, velodyne_gps.gyro3);
//	printf("Temp    -> 1: %6.2f, 2: %6.2f, 3: %6.2f\n", velodyne_gps.temp1, velodyne_gps.temp2, velodyne_gps.temp3);
//	printf("Accel_X -> 1: %6.2f, 2: %6.2f, 3: %6.2f\n", velodyne_gps.accel1_x, velodyne_gps.accel2_x, velodyne_gps.accel3_x);
//	printf("Accel_Y -> 1: %6.2f, 2: %6.2f, 3: %6.2f\n", velodyne_gps.accel1_y, velodyne_gps.accel2_y, velodyne_gps.accel3_y);
}

/*********************************************************
		   --- Publishers ---
**********************************************************/

/*void publish_velodyne_partial_scan()
{
	IPC_RETURN_TYPE err;

	err = IPC_publishData(CARMEN_VELODYNE_PARTIAL_SCAN_MESSAGE_NAME, &velodyne_partial_scan);
	carmen_test_ipc_exit(err, "Could not publish", CARMEN_VELODYNE_PARTIAL_SCAN_MESSAGE_FMT);
}*/

//void publish_velodyne_variable_scan()
//{
//	IPC_RETURN_TYPE err;
//
//	if (velodyne_number == 1)
//	{
//		err = IPC_publishData(CARMEN_VELODYNE_VARIABLE_SCAN_MESSAGE1_NAME, &velodyne_variable_scan);
//		carmen_test_ipc_exit(err, "Could not publish", CARMEN_VELODYNE_VARIABLE_SCAN_MESSAGE1_FMT);
//	}
//	else if (velodyne_number == 2)
//	{
//		err = IPC_publishData(CARMEN_VELODYNE_VARIABLE_SCAN_MESSAGE2_NAME, &velodyne_variable_scan);
//		carmen_test_ipc_exit(err, "Could not publish", CARMEN_VELODYNE_VARIABLE_SCAN_MESSAGE2_FMT);
//	}
//	else if (velodyne_number == 3)
//	{
//		err = IPC_publishData(CARMEN_VELODYNE_VARIABLE_SCAN_MESSAGE3_NAME, &velodyne_variable_scan);
//		carmen_test_ipc_exit(err, "Could not publish", CARMEN_VELODYNE_VARIABLE_SCAN_MESSAGE3_FMT);
//	}
//}

void publish_velodyne_gps(velodyne_driver::velodyne_gps_t gps)
{
	IPC_RETURN_TYPE err;

	assembly_velodyne_gps_message_from_gps(gps);

	err = IPC_publishData(CARMEN_VELODYNE_GPS_MESSAGE_NAME, &velodyne_gps);
	carmen_test_ipc_exit(err, "Could not publish", CARMEN_VELODYNE_GPS_MESSAGE_FMT);
}


/*********************************************************
		   --- Handlers ---
**********************************************************/


void
shutdown_module(int signo)
{
	if (signo == SIGINT)
	{
		carmen_ipc_disconnect();
		printf("\nLidar sensor: disconnected.\n");
		exit(0);
	}
}


/*********************************************************
		   --- Initializations ---
**********************************************************/


int read_parameters(int argc, char **argv)
{
	int num_items;

	carmen_param_t param_list_command_line[] =
	{
		{(char *) "commandline",	(char *) "velodyne_number",		CARMEN_PARAM_INT,	&(velodyne_number),		0, NULL},
	};
	carmen_param_allow_unfound_variables(1);
	carmen_param_install_params(argc, argv, param_list_command_line, sizeof(param_list_command_line) / sizeof(param_list_command_line[0]));

	char velodyne[10] = "velodyne";
	char integer_string[2];
	sprintf(integer_string, "%d", velodyne_number);
	strcat(velodyne, integer_string);

	carmen_param_t param_list[] = {
			{velodyne, (char*)"gps_enable", CARMEN_PARAM_ONOFF, &velodyne_gps_enabled, 0, NULL},

			{velodyne, (char*)"model", CARMEN_PARAM_STRING, &velodyne_model, 0, NULL},
			{velodyne, (char*)"velodyne_package_rate", CARMEN_PARAM_DOUBLE, &velodyne_package_rate, 0, NULL},
			{velodyne, (char*)"dist_lsb", CARMEN_PARAM_DOUBLE, &dist_lsb, 0, NULL},
			{velodyne, (char*)"rotation_resolution", CARMEN_PARAM_DOUBLE, &rotation_resolution, 0, NULL},
			{velodyne, (char*)"rotation_max_units", CARMEN_PARAM_INT, &rotation_max_units, 0, NULL},
			{velodyne, (char*)"velodyne_num_lasers", CARMEN_PARAM_INT, &velodyne_num_lasers, 0, NULL},
			{velodyne, (char*)"velodyne_num_shots", CARMEN_PARAM_INT, &velodyne_num_shots, 0, NULL},
			{velodyne, (char*)"min_sensing_distance", CARMEN_PARAM_INT, &min_sensing_distance, 0, NULL},
			{velodyne, (char*)"max_sensing_distance", CARMEN_PARAM_INT, &max_sensing_distance, 0, NULL},
			{velodyne, (char*)"velodyne_msg_buffer_size", CARMEN_PARAM_INT, &velodyne_msg_buffer_size, 0, NULL},
			{velodyne, (char*)"velodyne_gps_buffer_size", CARMEN_PARAM_INT, &velodyne_gps_buffer_size, 0, NULL},
			{velodyne, (char*)"velodyne_driver_broadcast_freq_hz", CARMEN_PARAM_DOUBLE, &velodyne_driver_broadcast_freq_hz, 0, NULL},
			{velodyne, (char*)"velodyne_udp_port", CARMEN_PARAM_INT, &velodyne_udp_port, 0, NULL},
			{velodyne, (char*)"velodyne_gps_udp_port", CARMEN_PARAM_INT, &velodyne_gps_udp_port, 0, NULL},
			{velodyne, (char*)"velodyne_upper_header_bytes", CARMEN_PARAM_INT, &velodyne_upper_header_bytes, 0, NULL},
			{velodyne, (char*)"velodyne_lower_header_bytes", CARMEN_PARAM_INT, &velodyne_lower_header_bytes, 0, NULL},
			{velodyne, (char*)"gyro_scale_factor", CARMEN_PARAM_DOUBLE, &gyro_scale_factor, 0, NULL},
			{velodyne, (char*)"temp_scale_factor", CARMEN_PARAM_DOUBLE, &temp_scale_factor, 0, NULL},
			{velodyne, (char*)"temp_base_factor", CARMEN_PARAM_INT, &temp_base_factor, 0, NULL},
			{velodyne, (char*)"accel_scale_factor", CARMEN_PARAM_DOUBLE, &accel_scale_factor, 0, NULL},
			{velodyne, (char*)"velodyne_min_frequency", CARMEN_PARAM_DOUBLE, &velodyne_min_frequency, 0, NULL}
	};
	num_items = sizeof(param_list)/sizeof(param_list[0]);
	carmen_param_install_params(argc, argv, param_list, num_items);

	velodyne_max_laser_shots_per_revolution  = (int) ((double) (velodyne_package_rate * velodyne_num_shots) * velodyne_min_frequency + 0.5);

	return 0;
}


void
read_parameters_new(int argc, char** argv, int lidar_id, carmen_lidar_config &lidar_config)
{
    char *vertical_correction_string;
    char lidar_string[256];
	
	sprintf(lidar_string, "lidar%d", lidar_id);        // Geather the lidar id

    carmen_param_t param_list[] = {
			{lidar_string, (char*)"model", CARMEN_PARAM_STRING, &lidar_config.model, 0, NULL},
			{lidar_string, (char*)"ip", CARMEN_PARAM_STRING, &lidar_config.ip, 0, NULL},
			{lidar_string, (char*)"port", CARMEN_PARAM_STRING, &lidar_config.port, 0, NULL},
			{lidar_string, (char*)"shot_size", CARMEN_PARAM_INT, &lidar_config.shot_size, 0, NULL},
            {lidar_string, (char*)"min_sensing", CARMEN_PARAM_INT, &lidar_config.min_sensing, 0, NULL},
            {lidar_string, (char*)"max_sensing", CARMEN_PARAM_INT, &lidar_config.max_sensing, 0, NULL},
			{lidar_string, (char*)"range_division_factor", CARMEN_PARAM_INT, &lidar_config.range_division_factor, 0, NULL},
            {lidar_string, (char*)"time_between_shots", CARMEN_PARAM_DOUBLE, &lidar_config.time_between_shots, 0, NULL},
			{lidar_string, (char*)"x", CARMEN_PARAM_DOUBLE, &(lidar_config.pose.position.x), 0, NULL},
			{lidar_string, (char*)"y", CARMEN_PARAM_DOUBLE, &(lidar_config.pose.position.y), 0, NULL},
			{lidar_string, (char*)"z", CARMEN_PARAM_DOUBLE, &lidar_config.pose.position.z, 0, NULL},
			{lidar_string, (char*)"roll", CARMEN_PARAM_DOUBLE, &lidar_config.pose.orientation.roll, 0, NULL},
			{lidar_string, (char*)"pitch", CARMEN_PARAM_DOUBLE, &lidar_config.pose.orientation.pitch, 0, NULL},
			{lidar_string, (char*)"yaw", CARMEN_PARAM_DOUBLE, &lidar_config.pose.orientation.yaw, 0, NULL},
			{lidar_string, (char*)"vertical_angles", CARMEN_PARAM_STRING, &vertical_correction_string, 0, NULL},
	};
	int num_items = sizeof(param_list) / sizeof(param_list[0]);
	carmen_param_install_params(argc, argv, param_list, num_items);

    lidar_config.vertical_angles = (double*) malloc(lidar_config.shot_size * sizeof(double));

    for (int i = 0; i < lidar_config.shot_size; i++)
		lidar_config.vertical_angles[i] = CLF_READ_DOUBLE(&vertical_correction_string); // CLF_READ_DOUBLE takes a double number from a string
    
	//printf("Model: %s Port: %s Shot size: %d Min Sensing: %d Max Sensing: %d Range division: %d Time: %lf\n", lidar_config.model, lidar_config.port, lidar_config.shot_size, lidar_config.min_sensing, lidar_config.max_sensing, lidar_config.range_division_factor, lidar_config.time_between_shots);  printf("X: %lf Y: %lf Z: %lf R: %lf P: %lf Y: %lf\n", lidar_config.pose.position.x, lidar_config.pose.position.y, lidar_config.pose.position.z, lidar_config.pose.orientation.roll, lidar_config.pose.orientation.pitch, lidar_config.pose.orientation.yaw); for (int i = 0; i < lidar_config.shot_size; i++) printf("%lf ", lidar_config.vertical_angles[i]); printf("\n");
}


void
setup_message(carmen_lidar_config lidar_config)
{
	variable_scan_msg.partial_scan = (carmen_velodyne_shot *) malloc (INITIAL_MAX_NUM_SHOT * sizeof(carmen_velodyne_shot));
	
    for (int i = 0 ; i < INITIAL_MAX_NUM_SHOT; i++)
	{
		variable_scan_msg.partial_scan[i].shot_size = lidar_config.shot_size;
		variable_scan_msg.partial_scan[i].distance = (unsigned short*) malloc (lidar_config.shot_size * sizeof(unsigned short));
		variable_scan_msg.partial_scan[i].intensity = (unsigned char*) malloc (lidar_config.shot_size * sizeof(unsigned char));
	}
	variable_scan_msg.host = carmen_get_host();
}


void
freeMemory()
{
	//free(velodyne_partial_scan.partial_scan);

	for (int i=0; i<(2 * velodyne_max_laser_shots_per_revolution); i++)
	{
		free(variable_scan_msg.partial_scan[i].distance);
		free(variable_scan_msg.partial_scan[i].intensity);
	}
	free(variable_scan_msg.partial_scan);
}


void
run_robosense_driver()
{
	private_nh.cut_angle = 359;
	private_nh.device_ip = "192.168.1.200";
	private_nh.difop_udp_port = 7788;
	private_nh.model = "RS16";
	private_nh.msop_port = 6699;
	private_nh.rpm = 1200; //pegar na conf
	private_nh.npackets = velodyne_max_laser_shots_per_revolution; // calcula dentro da mensagem e preenche se precisar depois

	variable_scan_msg.host = carmen_get_host();
	printf("Usando %s\n", velodyne_model);

	while (true)
	{
		if (robosense == NULL)
		{
			robosense = new rslidar_driver::rslidarDriver(variable_scan_msg, 16, velodyne_max_laser_shots_per_revolution, private_nh);
		}
		else
		{
			if(robosense->receive_socket_data_and_fill_message(variable_scan_msg, 16, velodyne_max_laser_shots_per_revolution, 6699, 7788, private_nh))
			{
//				printf("cheguei até aqui, respeita minha história\n");
				carmen_velodyne_publish_variable_scan_message(&variable_scan_msg, velodyne_number);
			}
			else
			{
				printf("robosense disconect\n");
				robosense->~rslidarDriver();
				freeMemory();
				velodyne = NULL;
				usleep(1e6 / 2);
			}
		}
	}
}


void
run_velodyne_driver()
{
	if (velodyne == NULL)
	{
		velodyne = new velodyne_driver::VelodyneDriver(variable_scan_msg, velodyne_num_lasers, velodyne_max_laser_shots_per_revolution, velodyne_udp_port, velodyne_gps_udp_port);
		config = velodyne->getVelodyneConfig();
	}
	else
	{
		if (velodyne->pollScan(variable_scan_msg, velodyne_number, velodyne_udp_port, velodyne_max_laser_shots_per_revolution, velodyne_num_shots,
				velodyne_package_rate, velodyne_num_lasers))
		{
			carmen_velodyne_publish_variable_scan_message(&variable_scan_msg, velodyne_number);

			if (velodyne_gps_enabled && velodyne->pollGps(velodyne_gps_udp_port))
			{
				gps = velodyne->getVelodyneGps();
				publish_velodyne_gps(gps);
			}
		}
		else // just in case velodyne crash
		{
			printf("velodyne disconect\n");
			velodyne->~VelodyneDriver();

			freeMemory();

			velodyne = NULL;
			usleep(1e6 / 2);
		}
	}
}


void
run_velodyne_driver_new(carmen_velodyne_variable_scan_message &msg, carmen_lidar_config lidar_config, int lidar_id)
{
	int port = atoi(lidar_config.port);

	if (velodyne == NULL)
	{
		velodyne = new velodyne_driver::VelodyneDriver(msg, lidar_config.shot_size, INITIAL_MAX_NUM_SHOT, port, 0);
		config = velodyne->getVelodyneConfig();
	}
	else
	{
		if (velodyne->pollScan(msg, lidar_id, port, INITIAL_MAX_NUM_SHOT, 12,
				velodyne_package_rate, lidar_config.shot_size))
		{
			carmen_velodyne_publish_variable_scan_message(&msg, lidar_id);

			// if (velodyne_gps_enabled && velodyne->pollGps(velodyne_gps_udp_port))
			// {
			// 	gps = velodyne->getVelodyneGps();
			// 	publish_velodyne_gps(gps);
			// }
		}
		else // just in case velodyne crash
		{
			printf("velodyne disconect\n");
			velodyne->~VelodyneDriver();

			freeMemory();

			velodyne = NULL;
			usleep(1e6 / 2);
		}
	}
}


int
get_lidar_id(int argc, char **argv)
{
	if (argc != 2)
		carmen_die("%s: Wrong number of parameters. %s requires 1 parameter and received %d parameter(s). \n\nUsage:\n %s <lidar_number>\n", argv[0], argv[0], argc-1, argv[0]);

	return (atoi(argv[1]));
}


int 
main(int argc, char **argv)
{
	int lidar_id;
	carmen_lidar_config lidar_config;

	carmen_ipc_initialize(argc, argv);

	carmen_param_check_version(argv[0]);

	signal(SIGINT, shutdown_module);

	//lidar_id = get_lidar_id(argc, argv);

	read_parameters(argc, argv);
	// read_parameters(argc, argv, lidar_id, lidar_config);

	carmen_velodyne_define_messages();

	printf("-------------------------\n  Lidar %d %s loaded!\n-------------------------\n", lidar_id, lidar_config.model);

	setup_message(lidar_config);

	if(strcmp(lidar_config.model, "RS16") == 0)
	{
		//run_robosense_driver();
		run_robosense_RSLiDAR16_driver(variable_scan_msg, lidar_config, lidar_id);
	}
	if(strcmp(lidar_config.model, "VLP16") == 0)
	{
		while (1)
			run_velodyne_driver();
			// run_velodyne_driver(variable_scan_msg, lidar_config, lidar_id);
	}

	carmen_die("\nERROR: lidar%d_model %s not found!\nPlease verify the carmen.ini file.\n\n", lidar_id, lidar_config.model);

	return 0;
}
