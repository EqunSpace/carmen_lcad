#ifndef UDATMO_OBSTACLE_H
#define UDATMO_OBSTACLE_H

#include "observation.h"

#include <deque>
#include <vector>

namespace udatmo
{

/**
 * @brief A moving obstacle.
 */
class Obstacle
{
	/** @brief Sequence of observations relating to this moving obstacle. */
	std::deque<Observation> track;

	/** @brief Number of missed observations of this moving obstacle. */
	int misses;

	/**
	 * @brief Update estimates of direction and speed of movement.
	 */
	void updateMovement();

public:
	enum Status {DETECTED, TRACKING, DROP};

	/** @brief RDDF index of the obstacle. */
	int index;

	/** @brief Instantaneous pose of the moving obstacle. */
	carmen_ackerman_traj_point_t pose;

	/**
	 * @brief Default constructor.
	 */
	Obstacle();

	Obstacle(const carmen_ackerman_traj_point_t &robot_pose, const Observation &observation);

	/**
	 * @brief Update this moving obstacle according to the given observation sequence.
	 */
	void update(const carmen_ackerman_traj_point_t &robot_pose, Observations &observed);

	/**
	 * @brief Add an observation to this moving obstacle.
	 */
	void update(const Observation &observation);

	double timestamp() const;

	Status status() const;
};

/** @brief Sequence of moving obstacles. */
typedef std::vector<Obstacle> Obstacles;

} // namespace udatmo

#endif
