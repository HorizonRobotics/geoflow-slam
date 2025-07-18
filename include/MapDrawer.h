/**
 * This file is part of ORB-SLAM3
 *
 * Copyright (C) 2017-2021 Carlos Campos, Richard Elvira, Juan J. Gómez
 * Rodríguez, José M.M. Montiel and Juan D. Tardós, University of Zaragoza.
 * Copyright (C) 2014-2016 Raúl Mur-Artal, José M.M. Montiel and Juan D. Tardós,
 * University of Zaragoza.
 *
 * ORB-SLAM3 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ORB-SLAM3 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ORB-SLAM3. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAPDRAWER_H
#define MAPDRAWER_H

#ifdef ENABLE_VIEWER
#include <pangolin/pangolin.h>
#endif
#include <mutex>

#include "Atlas.h"
#include "KeyFrame.h"
#include "MapPoint.h"
#include "Settings.h"

namespace ORB_SLAM3 {

class Settings;

class MapDrawer {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  MapDrawer(Atlas *pAtlas, const string &strSettingPath, Settings *settings);

  void newParameterLoader(Settings *settings);

  Atlas *mpAtlas;
#ifdef ENABLE_VIEWER
  void DrawMapPoints();
  void DrawKeyFrames(const bool bDrawKF, const bool bDrawGraph,
                     const bool bDrawInertialGraph, const bool bDrawOptLba);
  void DrawCurrentCamera(pangolin::OpenGlMatrix &Twc);
  void DrawCurrentCameraICP(pangolin::OpenGlMatrix &Twc);
  void SetCurrentCameraPose(const Sophus::SE3f &Tcw,const Sophus::SE3f &ICP_Twc= Sophus::SE3f());
  void GetCurrentOpenGLCameraMatrix(pangolin::OpenGlMatrix &M,
                                    pangolin::OpenGlMatrix &MOw);
  void GetCurrentICPOpenGLCameraMatrix(pangolin::OpenGlMatrix &M,
                                       pangolin::OpenGlMatrix &MOw);
#endif
 private:
  bool ParseViewerParamFile(cv::FileStorage &fSettings);

  float mKeyFrameSize;
  float mKeyFrameLineWidth;
  float mGraphLineWidth;
  float mPointSize;
  float mCameraSize;
  float mCameraLineWidth;

  Sophus::SE3f mCameraPose;
  Sophus::SE3f mCameraICPPose;

  std::mutex mMutexCamera;

  float mfFrameColors[6][3] = {{0.0f, 0.0f, 1.0f}, {0.8f, 0.4f, 1.0f},
                               {1.0f, 0.2f, 0.4f}, {0.6f, 0.0f, 1.0f},
                               {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}};
};

}  // namespace ORB_SLAM3

#endif  // MAPDRAWER_H
