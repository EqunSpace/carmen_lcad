
#ifndef _SEGMAP_POSE2D_H_
#define _SEGMAP_POSE2D_H_

#include <Eigen/Core>

using namespace Eigen;


class Pose2d
{
public:
	double x, y, th;

	Pose2d(double px = 0, double py = 0, double pth = 0);

	Pose2d operator=(Pose2d p);
	static double theta_from_matrix(Matrix<double, 4, 4> &m);

	static Pose2d from_matrix(Matrix<double, 4, 4> &m);
	static Matrix<double, 4, 4> to_matrix(Pose2d &p);
};


#endif
