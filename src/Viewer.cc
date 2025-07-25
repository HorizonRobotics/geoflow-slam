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

#include "Viewer.h"
#ifdef ENABLE_VIEWER
#include <pangolin/pangolin.h>
#endif
#include <sys/time.h>

#include <mutex>

namespace ORB_SLAM3 {

Viewer::Viewer(System *pSystem, FrameDrawer *pFrameDrawer,
               MapDrawer *pMapDrawer, Tracking *pTracking,
               const string &strSettingPath, Settings *settings)
    : both(false),
      mpSystem(pSystem),
      mpFrameDrawer(pFrameDrawer),
      mpMapDrawer(pMapDrawer),
      mpTracker(pTracking),
      mbFinishRequested(false),
      mbFinished(true),
      mbStopped(true),
      mbStopRequested(false),
      mpSettings(settings) {
  if (settings) {
    newParameterLoader(settings);
  } else {
    cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);

    bool is_correct = ParseViewerParamFile(fSettings);

    if (!is_correct) {
      std::cerr << "**ERROR in the config file, the format is not correct**"
                << std::endl;
      try {
        throw -1;
      } catch (exception &e) {
      }
    }
  }

  mbStopTrack = false;
}

void Viewer::newParameterLoader(Settings *settings) {
  mImageViewerScale = 1.f;

  float fps = settings->fps();
  if (fps < 1) fps = 30;
  mT = 1e3 / fps;

  cv::Size imSize = settings->newImSize();
  mImageHeight = imSize.height;
  mImageWidth = imSize.width;

  mImageViewerScale = settings->imageViewerScale();
  mViewpointX = settings->viewPointX();
  mViewpointY = settings->viewPointY();
  mViewpointZ = settings->viewPointZ();
  mViewpointF = settings->viewPointF();
}

bool Viewer::ParseViewerParamFile(cv::FileStorage &fSettings) {
  bool b_miss_params = false;
  mImageViewerScale = 1.f;

  float fps = fSettings["Camera.fps"];
  if (fps < 1) fps = 30;
  mT = 1e3 / fps;

  cv::FileNode node = fSettings["Camera.width"];
  if (!node.empty()) {
    mImageWidth = node.real();
  } else {
    std::cerr
        << "*Camera.width parameter doesn't exist or is not a real number*"
        << std::endl;
    b_miss_params = true;
  }

  node = fSettings["Camera.height"];
  if (!node.empty()) {
    mImageHeight = node.real();
  } else {
    std::cerr
        << "*Camera.height parameter doesn't exist or is not a real number*"
        << std::endl;
    b_miss_params = true;
  }

  node = fSettings["Viewer.imageViewScale"];
  if (!node.empty()) {
    mImageViewerScale = node.real();
  }

  node = fSettings["Viewer.ViewpointX"];
  if (!node.empty()) {
    mViewpointX = node.real();
  } else {
    std::cerr << "*Viewer.ViewpointX parameter doesn't exist or is not a "
                 "real number*"
              << std::endl;
    b_miss_params = true;
  }

  node = fSettings["Viewer.ViewpointY"];
  if (!node.empty()) {
    mViewpointY = node.real();
  } else {
    std::cerr << "*Viewer.ViewpointY parameter doesn't exist or is not a "
                 "real number*"
              << std::endl;
    b_miss_params = true;
  }

  node = fSettings["Viewer.ViewpointZ"];
  if (!node.empty()) {
    mViewpointZ = node.real();
  } else {
    std::cerr << "*Viewer.ViewpointZ parameter doesn't exist or is not a "
                 "real number*"
              << std::endl;
    b_miss_params = true;
  }

  node = fSettings["Viewer.ViewpointF"];
  if (!node.empty()) {
    mViewpointF = node.real();
  } else {
    std::cerr << "*Viewer.ViewpointF parameter doesn't exist or is not a "
                 "real number*"
              << std::endl;
    b_miss_params = true;
  }

  return !b_miss_params;
}

void Viewer::Run() {
  mbFinished = false;
  mbStopped = false;
#ifdef ENABLE_VIEWER
  pangolin::CreateWindowAndBind("Horizon-SLAM: Map Viewer", 1024, 768);

  // 3D Mouse handler requires depth testing to be enabled
  glEnable(GL_DEPTH_TEST);

  // Issue specific OpenGl we might need
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Define Camera Render Object (for view / scene browsing)
  // Define Camera Render Object (for view / scene browsing)
  pangolin::OpenGlRenderState s_cam(
      pangolin::ProjectionMatrix(mImageWidth, mImageHeight, 400, 400,
                                 mImageWidth / 2, mImageHeight / 2, 0.1, 1000),
      pangolin::ModelViewLookAt(-0, -5, -10, 0, 0, 0, pangolin::AxisNegY));

  // Add named OpenGL viewport to window and provide 3D Handler
  pangolin::View &d_cam =
      pangolin::CreateDisplay()
          .SetBounds(0.0, 1.0, pangolin::Attach::Pix(175), 1.0,
                     -mImageWidth / (float)mImageHeight)
          .SetHandler(new pangolin::Handler3D(s_cam));

  pangolin::View &d_video = pangolin::Display("imgVideo")
                                .SetAspect(mImageWidth / (float)mImageHeight);

  pangolin::GlTexture texVideo(mImageWidth, mImageHeight, GL_RGB, false, 0,
                               GL_RGB, GL_UNSIGNED_BYTE);

  pangolin::CreateDisplay()
      .SetBounds(0.0, 0.45, pangolin::Attach::Pix(175), 1.0)
      .SetLayout(pangolin::LayoutEqual)
      .AddDisplay(d_video);

  pangolin::CreatePanel("menu").SetBounds(0.0, 1.0, 0.0,
                                          pangolin::Attach::Pix(175));
  pangolin::Var<bool> menuFollowCamera("menu.Follow Camera", true, true);
  pangolin::Var<bool> menuCamView("menu.Camera View", false, false);
  pangolin::Var<bool> menuTopView("menu.Top View", true, false);
  // pangolin::Var<bool> menuSideView("menu.Side View",false,false);
  pangolin::Var<bool> menuShowPoints("menu.Show Points", true, true);
  pangolin::Var<bool> menuShowKeyFrames("menu.Show KeyFrames", true, true);
  pangolin::Var<bool> menuShowGraph("menu.Show Graph", false, true);
  pangolin::Var<bool> menuShowInertialGraph("menu.Show Inertial Graph", true,
                                            true);
  pangolin::Var<bool> menuLocalizationMode("menu.Localization Mode", false,
                                           true);
  pangolin::Var<bool> menuReset("menu.Reset", false, false);
  pangolin::Var<bool> menuStop("menu.Stop", false, false);
  pangolin::Var<bool> menuStepByStep("menu.Step By Step", false,
                                     true);  // false, true
  pangolin::Var<bool> menuStep("menu.Step", false, false);

  pangolin::Var<bool> menuShowOptLba("menu.Show LBA opt", false, true);
  pangolin::Var<bool> menuShowImg("menu.Show image", true, true);

  if (mpSettings->get_sensor() == mpSystem->IMU_RGBD ||
      mpSettings->get_sensor() == mpSystem->IMU_MONOCULAR ||
      mpSettings->get_sensor() == mpSystem->IMU_STEREO) {
    pangolin::Var<int> menuIMUMethod("menu.Imu method", 1, 0, 2, false);
    menuIMUMethod = mpSettings->getImuInitMethod();
    menuIMUMethod.Get();
  }
  pangolin::Var<string> menuState("menu.State", "OK", false);
  pangolin::Var<string> menuWarning("menu.Warning", "Tracking is normal",
                                    false);
  string lastMenuState = menuState.Get();

  pangolin::OpenGlMatrix Twc, Twr, Twc_icp;
  // pangolin::OpenGlMatrix Twc, Twr;
  Twc.SetIdentity();
  Twc_icp.SetIdentity();
  // pangolin::OpenGlMatrix Ow;  // Oriented with g in the z axis
  pangolin::OpenGlMatrix Ow, Ow_icp;  // Oriented with g in the z axis
  Ow.SetIdentity();
  Ow_icp.SetIdentity();
  cv::namedWindow("Horizon-SLAM: Current Frame");

  bool bFollow = true;
  bool bLocalizationMode = false;
  bool bStepByStep = false;
  bool bCameraView = true;
  cv::Mat toShow(
      cv::Size(mImageHeight, mImageWidth),
      CV_8UC3);  // = cv::Mat::zeros(mImageHeight, mImageWidth, CV_8UC3);

  if (mpTracker->mSensor == mpSystem->MONOCULAR ||
      mpTracker->mSensor == mpSystem->STEREO ||
      mpTracker->mSensor == mpSystem->RGBD) {
    menuShowGraph = true;
  }

  float trackedImageScale = mpTracker->GetImageScale();

  cout << "Starting the Viewer" << endl;
  while (1) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    mpMapDrawer->GetCurrentOpenGLCameraMatrix(Twc, Ow);
    mpMapDrawer->GetCurrentICPOpenGLCameraMatrix(Twc_icp, Ow_icp);
    switch (mpTracker->mState) {
      case Tracking::SYSTEM_NOT_READY:
        menuState = "SYSTEM_NOT_READY";
        break;
      case Tracking::NO_IMAGES_YET:
        menuState = "NO_IMAGES_YET";
        menuWarning = "Waiting for images";
        break;
      case Tracking::NOT_INITIALIZED:
        menuState = "NOT_INITIALIZED";
        menuWarning = "Trying to initialize";
        break;
      case Tracking::OK:
        menuState = "OK";
        menuWarning = "Tracking is OK!";
        break;
      case Tracking::RECENTLY_LOST:
        menuState = "RECENTLY_LOST";
        menuWarning = "Tracking is RECENTLY_LOST";
        break;
      case Tracking::LOST:
        menuState = "LOST";
        menuWarning = "Tracking is lost!";
        break;
      case Tracking::OK_KLT:
        menuState = "OK_KLT";
        break;
      default:
        menuState = "UNKNOWN";
        break;
    }
    menuWarning.Get();
    if (mbStopTrack) {
      menuStepByStep = true;
      mbStopTrack = false;
    }

    if (menuFollowCamera && bFollow) {
      if (bCameraView)
        s_cam.Follow(Twc);
      else
        s_cam.Follow(Ow);
    } else if (menuFollowCamera && !bFollow) {
      if (bCameraView) {
        s_cam.SetProjectionMatrix(pangolin::ProjectionMatrix(
            1024, 768, mViewpointF, mViewpointF, 512, 389, 0.1, 1000));
        s_cam.SetModelViewMatrix(pangolin::ModelViewLookAt(
            mViewpointX, mViewpointY, mViewpointZ, 0, 0, 0, 0.0, -1.0, 0.0));
        s_cam.Follow(Twc);
      } else {
        s_cam.SetProjectionMatrix(pangolin::ProjectionMatrix(
            1024, 768, 3000, 3000, 512, 389, 0.1, 1000));
        s_cam.SetModelViewMatrix(
            pangolin::ModelViewLookAt(0, 0.01, 10, 0, 0, 0, 0.0, 0.0, 1.0));
        s_cam.Follow(Ow);
      }
      bFollow = true;
    } else if (!menuFollowCamera && bFollow) {
      bFollow = false;
    }

    if (menuCamView) {
      menuCamView = false;
      bCameraView = true;
      s_cam.SetProjectionMatrix(pangolin::ProjectionMatrix(
          1024, 768, mViewpointF, mViewpointF, 512, 389, 0.1, 10000));
      s_cam.SetModelViewMatrix(pangolin::ModelViewLookAt(
          mViewpointX, mViewpointY, mViewpointZ, 0, 0, 0, 0.0, -1.0, 0.0));
      s_cam.Follow(Twc);
    }

    if (menuTopView && mpMapDrawer->mpAtlas->isImuInitialized()) {
      menuTopView = false;
      bCameraView = false;
      s_cam.SetProjectionMatrix(pangolin::ProjectionMatrix(
          1024, 768, 3000, 3000, 512, 389, 0.1, 10000));
      s_cam.SetModelViewMatrix(
          pangolin::ModelViewLookAt(0, 0.01, 50, 0, 0, 0, 0.0, 0.0, 1.0));
      s_cam.Follow(Ow);
    }

    if (menuLocalizationMode && !bLocalizationMode) {
      mpSystem->ActivateLocalizationMode();
      bLocalizationMode = true;
    } else if (!menuLocalizationMode && bLocalizationMode) {
      mpSystem->DeactivateLocalizationMode();
      bLocalizationMode = false;
    }

    if (menuStepByStep && !bStepByStep) {
      // cout << "Viewer: step by step" << endl;
      mpTracker->SetStepByStep(true);
      bStepByStep = true;
    } else if (!menuStepByStep && bStepByStep) {
      mpTracker->SetStepByStep(false);
      bStepByStep = false;
    }

    if (menuStep) {
      mpTracker->mbStep = true;
      menuStep = false;
    }
    {
      d_cam.Activate(s_cam);
      // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      mpMapDrawer->DrawCurrentCamera(Twc);
      mpMapDrawer->DrawCurrentCameraICP(Twc_icp);
      if (menuShowKeyFrames || menuShowGraph || menuShowInertialGraph ||
          menuShowOptLba)
        mpMapDrawer->DrawKeyFrames(menuShowKeyFrames, menuShowGraph,
                                   menuShowInertialGraph, menuShowOptLba);
      if (menuShowPoints) mpMapDrawer->DrawMapPoints();
    }
    pangolin::FinishFrame();

    cv::Mat toShow;
    cv::Mat im = mpFrameDrawer->DrawFrame(trackedImageScale);

    if (both) {
      cv::Mat imRight = mpFrameDrawer->DrawRightFrame(trackedImageScale);
      cv::hconcat(im, imRight, toShow);
    } else {
      toShow = im;
    }

    if (mImageViewerScale != 1.f) {
      int width = toShow.cols * mImageViewerScale;
      int height = toShow.rows * mImageViewerScale;
      cv::resize(toShow, toShow, cv::Size(width, height));
    }

    cv::imshow("Horizon-SLAM: Current Frame", toShow);
    cv::waitKey(mT);
    if (menuReset) {
      menuShowGraph = true;
      menuShowInertialGraph = true;
      menuShowKeyFrames = true;
      menuShowPoints = true;
      menuLocalizationMode = false;
      if (bLocalizationMode) mpSystem->DeactivateLocalizationMode();
      bLocalizationMode = false;
      bFollow = true;
      menuFollowCamera = true;
      // mpSystem->ResetActiveMap();
      mpSystem->Reset();
      menuReset = false;
      menuShowImg = true;
    }

    if (menuStop) {
      if (bLocalizationMode) mpSystem->DeactivateLocalizationMode();

      // Stop all threads
      mpSystem->Shutdown();

      // Save camera trajectory
      mpSystem->SaveTrajectoryEuRoC("CameraTrajectory.txt");
      mpSystem->SaveKeyFrameTrajectoryEuRoC("KeyFrameTrajectory.txt");
      menuStop = false;
    }

    if (Stop()) {
      while (isStopped()) {
        usleep(3000);
      }
    }

    if (CheckFinish()) break;
    pangolin::FinishFrame();
    usleep(25000);
  }
#else
  static int toShowId = 0;
  while (true) {
    if (false) {
      float trackedImageScale = mpTracker->GetImageScale();
      cv::Mat toShow;
      cv::Mat im = mpFrameDrawer->DrawFrame(trackedImageScale);
      if (both) {
        cv::Mat imRight = mpFrameDrawer->DrawRightFrame(trackedImageScale);
        cv::hconcat(im, imRight, toShow);
      } else {
        toShow = im;
      }
      if (mImageViewerScale != 1.f) {
        int width = toShow.cols * mImageViewerScale;
        int height = toShow.rows * mImageViewerScale;
        cv::resize(toShow, toShow, cv::Size(width, height));
      }
      cv::imwrite("render/" + std::to_string(toShowId++) + ".png", toShow);
    } else {
      usleep(1000 * 1000);
    }

    if (Stop()) {
      while (isStopped()) {
        usleep(3000);
      }
    }
    if (CheckFinish()) {
      break;
    }
  }
#endif
  SetFinish();
}

void Viewer::RequestFinish() {
  unique_lock<mutex> lock(mMutexFinish);
  mbFinishRequested = true;
}

bool Viewer::CheckFinish() {
  unique_lock<mutex> lock(mMutexFinish);
  return mbFinishRequested;
}

void Viewer::SetFinish() {
  unique_lock<mutex> lock(mMutexFinish);
  mbFinished = true;
}

bool Viewer::isFinished() {
  unique_lock<mutex> lock(mMutexFinish);
  return mbFinished;
}

void Viewer::RequestStop() {
  unique_lock<mutex> lock(mMutexStop);
  if (!mbStopped) mbStopRequested = true;
}

bool Viewer::isStopped() {
  unique_lock<mutex> lock(mMutexStop);
  return mbStopped;
}

bool Viewer::Stop() {
  unique_lock<mutex> lock(mMutexStop);
  unique_lock<mutex> lock2(mMutexFinish);

  if (mbFinishRequested)
    return false;
  else if (mbStopRequested) {
    mbStopped = true;
    mbStopRequested = false;
    return true;
  }

  return false;
}

void Viewer::Release() {
  unique_lock<mutex> lock(mMutexStop);
  mbStopped = false;
}

/*void Viewer::SetTrackingPause()
{
    mbStopTrack = true;
}*/

}  // namespace ORB_SLAM3
