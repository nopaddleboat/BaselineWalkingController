#include <mc_rtc/gui/Button.h>
#include <mc_tasks/CoMTask.h>
#include <mc_tasks/OrientationTask.h>

#include <BaselineWalkingController/BaselineWalkingController.h>
#include <BaselineWalkingController/CentroidalManager.h>
#include <BaselineWalkingController/FootManager.h>
#include <BaselineWalkingController/states/InitialState.h>
#include <BaselineWalkingController/tasks/FirstOrderImpedanceTask.h>

using namespace BWC;

void InitialState::start(mc_control::fsm::Controller & _ctl)
{
  State::start(_ctl);

  phase_ = 0;

  // Setup GUI
  ctl().gui()->addElement({"BWC"}, mc_rtc::gui::Button("Start", [this]() { phase_ = 1; }));

  output("OK");
}

bool InitialState::run(mc_control::fsm::Controller &)
{
  if(phase_ == 0)
  {
    // Auto start
    if(config_.has("configs") && config_("configs").has("autoStartTime")
       && ctl().t() > static_cast<double>(config_("configs")("autoStartTime")))
    {
      phase_ = 1;
    }

    return false;
  }
  if(phase_ == 1)
  {
    phase_ = 2;

    // Clean up GUI
    ctl().gui()->removeElement({"BWC"}, "Start");

    // Reset and add tasks
    ctl().comTask_->reset();
    ctl().solver().addTask(ctl().comTask_);
    ctl().baseOriTask_->reset();
    ctl().solver().addTask(ctl().baseOriTask_);
    for(const auto & foot : Feet::Both)
    {
      ctl().footTasks_.at(foot)->reset();
      ctl().solver().addTask(ctl().footTasks_.at(foot));
    }

    // Reset managers
    ctl().footManager_->reset();
    ctl().centroidalManager_->reset();
    ctl().enableManagerUpdate_ = true;

    // Setup anchor frame
    ctl().centroidalManager_->setAnchorFrame();

    // Add GUI of managers
    ctl().footManager_->addToGUI(*ctl().gui());
    ctl().centroidalManager_->addToGUI(*ctl().gui());

    return false;
  }
  else // if(phase_ == 2)
  {
    phase_ = 3;

    // Add logger of managers
    // Considering the possibility that logger entries assume that variables are set in the manager's update method,
    // it is safe to call the update method once and then add the logger
    ctl().footManager_->addToLogger(ctl().logger());
    ctl().centroidalManager_->addToLogger(ctl().logger());

    return true;
  }
}

void InitialState::teardown(mc_control::fsm::Controller &) {}

EXPORT_SINGLE_STATE("BWC::Initial", InitialState)
