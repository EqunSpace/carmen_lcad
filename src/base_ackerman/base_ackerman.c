/*********************************************************
 *
 * This source code is part of the Carnegie Mellon Robot
 * Navigation Toolkit (CARMEN)
 *
 * CARMEN Copyright (c) 2002 Michael Montemerlo, Nicholas
 * Roy, Sebastian Thrun, Dirk Haehnel, Cyrill Stachniss,
 * and Jared Glover
 *
 * CARMEN is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public 
 * License as published by the Free Software Foundation; 
 * either version 2 of the License, or (at your option)
 * any later version.
 *
 * CARMEN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General 
 * Public License along with CARMEN; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, 
 * Suite 330, Boston, MA  02111-1307 USA
 *
 ********************************************************/

#include <carmen/carmen.h>
#include <carmen/robot_ackerman_interface.h>
#include "base_ackerman.h"
#include "base_ackerman_simulation.h"
#include "base_ackerman_messages.h"


// Global variables

static carmen_base_ackerman_config_t *car_config = NULL;

static double phi_multiplier;
static double phi_bias;
static double v_multiplier;


//////////////////////////////////////////////////////////////////////////////////////////////////
//																								//
// Publishers																					//
//																								//
//////////////////////////////////////////////////////////////////////////////////////////////////


static void
publish_carmen_base_ackerman_odometry_message(double timestamp)
{
	IPC_RETURN_TYPE err = IPC_OK;
	static carmen_base_ackerman_odometry_message odometry;
	static int first = 1;
	static double first_timestamp;
	static FILE *graf;

	if (first)
	{
		odometry.host = carmen_get_host();
		odometry.x = 0;
		odometry.y = 0;
		odometry.theta = 0;

		odometry.v = odometry.phi = 0;
		first_timestamp = timestamp;

		graf = fopen("odometry_graph.txt", "w");
		first = 0;
	}

	odometry.x = car_config->odom_pose.x;
	odometry.y = car_config->odom_pose.y;
	odometry.theta = car_config->odom_pose.theta;
	odometry.v = car_config->v;
	odometry.phi = car_config->phi;
	odometry.timestamp = timestamp;

	fprintf(graf, "v_phi_time %lf %lf %lf\n", odometry.v, -odometry.phi, odometry.timestamp - first_timestamp); // @@@ Alberto: O phi esta negativado porque o carro inicialmente publicava a odometria ao contrario

	err = IPC_publishData(CARMEN_BASE_ACKERMAN_ODOMETRY_NAME, &odometry);
	carmen_test_ipc(err, "Could not publish base_odometry_message", CARMEN_BASE_ACKERMAN_ODOMETRY_NAME);
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//																								//
// Handlers																						//
//																								//
//////////////////////////////////////////////////////////////////////////////////////////////////


static void
robot_ackerman_velocity_handler(carmen_robot_ackerman_velocity_message *robot_ackerman_velocity_message)
{

	carmen_add_bias_and_multiplier_to_v_and_phi(&(car_config->v), &(car_config->phi),
						robot_ackerman_velocity_message->v, robot_ackerman_velocity_message->phi,
						0.0, v_multiplier, phi_bias, phi_multiplier);
	// Filipe: Nao deveria ter um normalize theta nessa atualizacao do phi? Sugestao abaixo:
	// car_config->phi = carmen_normalize_theta(robot_ackerman_velocity_message->phi * phi_multiplier + phi_bias);

	carmen_simulator_ackerman_recalc_pos(car_config);

	if (car_config->publish_odometry)
		publish_carmen_base_ackerman_odometry_message(robot_ackerman_velocity_message->timestamp);
}


static void 
shutdown_module()
{
	static int done = 0;

	if (!done)
	{
		carmen_ipc_disconnect();
		printf("base_ackerman disconnected from IPC.\n");
		done = 1;
	}
	exit(0);
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//																								//
// Inicializations																				//
//																								//
//////////////////////////////////////////////////////////////////////////////////////////////////


static void 
read_parameters(int argc, char *argv[], carmen_base_ackerman_config_t *config)
{
	int num_items;

	carmen_param_t param_list[]= 
	{
		{"robot", "width", CARMEN_PARAM_DOUBLE, &(config->width), 1, NULL},
		{"robot", "length", CARMEN_PARAM_DOUBLE, &(config->length), 1, NULL},
		{"robot", "distance_between_front_and_rear_axles", CARMEN_PARAM_DOUBLE, &(config->distance_between_front_and_rear_axles), 1,NULL},

		{"robot", "publish_odometry", CARMEN_PARAM_ONOFF, &(config->publish_odometry), 0, NULL},

		{"robot", "phi_multiplier", CARMEN_PARAM_DOUBLE, &phi_multiplier, 0, NULL},
		{"robot", "phi_bias", CARMEN_PARAM_DOUBLE, &phi_bias, 1, NULL},
		{"robot", "v_multiplier", CARMEN_PARAM_DOUBLE, &v_multiplier, 0, NULL}
	};

	num_items = sizeof(param_list) / sizeof(param_list[0]);
	carmen_param_install_params(argc, argv, param_list, num_items);
}


static void
subscribe_to_relevant_messages()
{
	  carmen_robot_ackerman_subscribe_velocity_message(NULL, (carmen_handler_t) robot_ackerman_velocity_handler, CARMEN_SUBSCRIBE_LATEST);
}


static int 
initialize_ipc(void)
{
	IPC_RETURN_TYPE err;

	err = IPC_defineMsg(CARMEN_BASE_ACKERMAN_ODOMETRY_NAME, IPC_VARIABLE_LENGTH, CARMEN_BASE_ACKERMAN_ODOMETRY_FMT);
	carmen_test_ipc_exit(err, "Could not define message", CARMEN_BASE_ACKERMAN_ODOMETRY_NAME);

	return 0;
}


static void
initialize_structures()
{
	// Init relevant data strutures
	car_config = malloc(sizeof(carmen_base_ackerman_config_t));
	memset(car_config, 0, sizeof(carmen_base_ackerman_config_t));
}


int 
main(int argc, char** argv)
{
	signal(SIGINT, shutdown_module);

	carmen_ipc_initialize(argc, argv);
	carmen_param_check_version(argv[0]);

	initialize_structures();

	read_parameters(argc, argv, car_config);

	initialize_ipc();

	subscribe_to_relevant_messages();

	carmen_ipc_dispatch();

	return 0;
}
