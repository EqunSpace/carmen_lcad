/*!@file Robots2/Beobot2/BeoWebServer.H Ice Module to Log data    */
// //////////////////////////////////////////////////////////////////// //
// The iLab Neuromorphic Vision C++ Toolkit - Copyright (C) 2001 by the //
// University of Southern California (USC) and the iLab at USC.         //
// See http://iLab.usc.edu for information about this project.          //
// //////////////////////////////////////////////////////////////////// //
// Major portions of the iLab Neuromorphic Vision Toolkit are protected //
// under the U.S. patent ``Computation of Intrinsic Perceptual Saliency //
// in Visual Environments, and Applications'' by Christof Koch and      //
// Laurent Itti, California Institute of Technology, 2001 (patent       //
// pending; application number 09/912,225 filed July 23, 2001; see      //
// http://pair.uspto.gov/cgi-bin/final/home.pl for current status).     //
// //////////////////////////////////////////////////////////////////// //
// This file is part of the iLab Neuromorphic Vision C++ Toolkit.       //
//                                                                      //
// The iLab Neuromorphic Vision C++ Toolkit is free software; you can   //
// redistribute it and/or modify it under the terms of the GNU General  //
// Public License as published by the Free Software Foundation; either  //
// version 2 of the License, or (at your option) any later version.     //
//                                                                      //
// The iLab Neuromorphic Vision C++ Toolkit is distributed in the hope  //
// that it will be useful, but WITHOUT ANY WARRANTY; without even the   //
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      //
// PURPOSE.  See the GNU General Public License for more details.       //
//                                                                      //
// You should have received a copy of the GNU General Public License    //
// along with the iLab Neuromorphic Vision C++ Toolkit; if not, write   //
// to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,   //
// Boston, MA 02111-1307 USA.                                           //
// //////////////////////////////////////////////////////////////////// //
//
// Primary maintainer for this file: Chin-Kai Chang<chinkaic@usc.edu>
// $HeadURL: svn://ilab.usc.edu/trunk/saliency/src/Robots/Beobot2/Service/BeoWebServer.H
// $Id: BeoWebServer.H 15190 2012-02-29 04:00:13Z beobot $
//

#ifndef BEOWEBSERVER_H
#define BEOWEBSERVER_H

#include "Component/ModelComponent.H"
#include "Component/ModelParam.H"

#include "Robots/RobotBrain/RobotBrainComponent.H"
#include "Util/Timer.H"

#include "Ice/RobotBrainObjects.ice.H"
#include "Ice/RobotSimEvents.ice.H"
#include "Ice/BeobotEvents.ice.H"
#include "mongoose.h"
#include <IceUtil/Thread.h>

#include <vector>

class BeoWebServer : public RobotBrainComponent
{
public:

  BeoWebServer(OptionManager& mgr,
         const std::string& descrName = "BeoWebServer",
         const std::string& tagName = "BeoWebServer");

  ~BeoWebServer();

  virtual void evolve();

  //! Get a message
  virtual void updateMessage
  (const RobotSimEvents::EventMessagePtr& eMsg,
   const Ice::Current&);

  virtual void registerTopics();

  // initialize the log folder
  void start1();

  // publish the log folder name
  void start3();
	void *HTTPRequestCallback(enum mg_event event,
			struct mg_connection *conn,
			const struct mg_request_info *request_info);
	std::string getLogFileName(){return itsLogFilename;}
	std::string getLogFolderName(){return itsLogFolderName;}
	std::vector<byte> getJpgImg();
  void updateMotor(double tran, double rot);
	int test;

  std::string itsImuString;
  std::string itsLrfString;
  std::string itsMotorString;
  std::string itsGpsString;
private:
  void writeToLogFile(std::string line);

  bool initLogFile();
  bool broadcastLogFileName();

  std::string itsLogFilename;
  IceUtil::Mutex its_logFilename_mutex;        //!< locking log filename
  IceUtil::Mutex its_JpgImg_mutex;        //!< locking log filename

  //std::vector<LoggerData> itsData;
  //nub::soft_ref<OutputFrameSeries> itsOfs;
  Timer itsTimer;
  std::string itsStartTime;
  std::string itsLogFolderName;
  int itsCurrMessageID;
	std::vector<byte> itsJpgImg;

};

#endif


// ######################################################################
/* So things look consistent in everyone's emacs... */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* End: */
