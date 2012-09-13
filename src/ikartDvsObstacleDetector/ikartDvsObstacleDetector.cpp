/* 
 * Copyright (C)2011  Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author: Marco Randazzo
 * email:  marco.randazzo@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/Os.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Vector.h>
#include <yarp/dev/Drivers.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/RateThread.h>
#include <yarp/dev/IAnalogSensor.h>
#include <string>
#include <math.h>

#include "ikartDVSObstacleDetector.h"

using namespace std;
using namespace yarp::os;
using namespace yarp::dev;

void obstacleDetectorThread::clearScan()
{
	for (int i=0; i<1080; i++)
	{
		scan_data[i]=100;
	}
}

int deg2las (double deg)
{
	return (int)(deg/270*1080);
}

double las2deg (int las)
{
	return double(las)/1080*270;
}

void image2world (int x, int y, double &d, double& ang)
{
	double ymin=1; //m
	double ymax=3.5; //m
	double xpixels = 128;
	double ypixels = 128;
	d=(y/ypixels*(ymax-ymin)) + ymin;
	ang=0;
}

void obstacleDetectorThread::compute_scan_1(double detected_distance)
{
	double alpha     = atan2(obstacle_size/2,detected_distance)*180.0/M_PI;
	int alpha_i      = deg2las(alpha);
	int alpha_start  = 1080/2-alpha_i;
	int alpha_end    = 1080/2+alpha_i;
	for (int i=0; i<alpha_i*2; i++)
	{
		int index = alpha_start+i;
		double t = 270/2-las2deg(index);
		double coeff = cos(t/180.0*M_PI);
		double curr_d = detected_distance / coeff;
		scan_data[index] = curr_d;
		//fprintf (stdout, "%d %f\n", i, scan_data[index]);
	}
}

void obstacleDetectorThread::updateIkartVel()
{
	Bottle *b_ikart_vel = port_ikart_velocity_input.read(false);
	if (b_ikart_vel)
	{
#if USE_IKART_COMMANDS
		//this block is used if the velocity is taken from the ikart input commands
		if (b_ikart_vel->get(0).asInt() == 0)
		{
			ikart_vx = b_ikart_vel->get(1).asDouble();
			ikart_vy = b_ikart_vel->get(2).asDouble();
			ikart_vt = b_ikart_vel->get(3).asDouble();
			last_data = yarp::os::Time::now();
		}
		else
		{
			printf ("Invalid input command format! \n");
			ikart_vx = ikart_vy = ikart_vt = 0;
		}
#else
		//this block is used if the velocity is taken from the ikart odometry
		ikart_vx = b_ikart_vel->get(3).asDouble();
		ikart_vy = b_ikart_vel->get(4).asDouble();
		ikart_vt = b_ikart_vel->get(5).asDouble();
		last_data = yarp::os::Time::now();
#endif
	}
	double curr_time = yarp::os::Time::now();
	if (curr_time - last_data > 0.3) 
	{
		printf ("Input command timeout! \n");
		ikart_vx = ikart_vy = ikart_vt = 0;
	}
}

void obstacleDetectorThread::run()
{
	//get the ikart velocity
	updateIkartVel();

	//get the optical flow buffer;
	VelocityBuffer* buff = port_buffered_optical_flow_input.read(false);
	if (buff)
	{
		optical_flow_buffer = *buff;
	}

	//compute the scan 
	clearScan();
	double detected_distance = 1; //m
	compute_scan_1(detected_distance);

	//send the simulated obstacle
	if (port_simulated_scan_output.getOutputCount()>0)
	{
		yarp::sig::Vector &v=port_simulated_scan_output.prepare();
		v=scan_data;
		port_simulated_scan_output.write();
	}

	//send the optical flow model image
	if (port_flow_model_output.getOutputCount()>0)
	{
		flow_model.redraw();
		yarp::sig::ImageOf<yarp::sig::PixelMono16>& img=port_flow_model_output.prepare();
		img=flow_model.flow_model_image;
		port_flow_model_output.write();
	}
}

void obstacleDetectorThread::printStats()
{
}
