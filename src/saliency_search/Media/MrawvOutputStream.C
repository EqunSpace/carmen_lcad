/*!@file Media/MrawvOutputStream.C MrawvOutputStream writes multiple multi-frame rawvideo files */

// //////////////////////////////////////////////////////////////////// //
// The iLab Neuromorphic Vision C++ Toolkit - Copyright (C) 2000-2005   //
// by the University of Southern California (USC) and the iLab at USC.  //
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
// Primary maintainer for this file: Rob Peters <rjpeters at usc dot edu>
// $HeadURL: svn://isvn.usc.edu/software/invt/trunk/saliency/src/Media/MrawvOutputStream.C $
// $Id: MrawvOutputStream.C 8946 2007-11-06 01:08:41Z rjpeters $
//

#ifndef MEDIA_MRAWVOUTPUTSTREAM_C_DEFINED
#define MEDIA_MRAWVOUTPUTSTREAM_C_DEFINED

#include "Media/MrawvOutputStream.H"

#include "Media/MrawvEncoder.H"
#include "Transport/TransportOpts.H" // for MOC_OUTPUT

// Used by: OutputFrameSeries
static const ModelOptionDef OPT_MrawvOutputScale255 =
  { MODOPT_FLAG, "MrawvOutputScale255", &MOC_OUTPUT, OPTEXP_CORE,
    "Whether to rescale floating-point outputs into a 0-255 range "
    "when they are sent to --out=mraw",
    "mraw-output-scale-255", '\0', "", "false" };

// ######################################################################
MrawvOutputStream::MrawvOutputStream(OptionManager& mgr,
                                     const std::string& descrName,
                                     const std::string& tagName)
  :
  LowLevelEncoderMap(mgr, descrName, tagName),
  itsScale255(&OPT_MrawvOutputScale255, this),
  itsStem()
{}

// ######################################################################
MrawvOutputStream::~MrawvOutputStream()
{}

// ######################################################################
void MrawvOutputStream::setConfigInfo(const std::string& filestem)
{
  // NOTE: if you modify any behavior here, then please update the
  // corresponding documentation for the global "--out" option inside
  // the OPT_OutputFrameSink definition in Media/MediaOpts.C

  this->setFileStem(filestem);
}

// ######################################################################
void MrawvOutputStream::setFileStem(const std::string& s)
{
  itsStem = s;
}

// ######################################################################
rutz::shared_ptr<LowLevelEncoder>
MrawvOutputStream::makeEncoder(const GenericFrameSpec& spec,
                               const std::string& shortname,
                               const FrameInfo& auxinfo)
{
  // NOTE: for now, we just ignore the FrameInfo auxinfo here; could
  // we use it to embed a comment in the mgz file, though?

  return rutz::shared_ptr<MrawvEncoder>
    (new MrawvEncoder(spec, itsStem+shortname, itsScale255.getVal()));
}

// ######################################################################
/* So things look consistent in everyone's emacs... */
/* Local Variables: */
/* mode: c++ */
/* indent-tabs-mode: nil */
/* End: */

#endif // MEDIA_MRAWVOUTPUTSTREAM_C_DEFINED
