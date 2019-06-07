/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/background_helper_android.h"

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace brave_ads {

BackgroundHelperAndroid::BackgroundHelperAndroid() {
  app_status_listener_  = (base::android::ApplicationStatusListener::New(
      base::BindRepeating(&BackgroundHelperAndroid::OnApplicationStateChange, 
      AsWeakPtr())));
  last_state_ = base::android::ApplicationStatusListener::GetState();
}

BackgroundHelperAndroid::~BackgroundHelperAndroid() {
  app_status_listener_.reset();
}

void BackgroundHelperAndroid::OnApplicationStateChange(base::android::ApplicationState state) {
  if ( base::android::ApplicationState::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES == state) {
     TriggerOnForeground();
  }
  // ignoring diff between APPLICATION_STATE_HAS_PAUSED_ACTIVITIES and 
  // APPLICATION_STATE_HAS_STOPPED_ACTIVITIES
  else if (last_state_ != state && last_state_ ==
      base::android::ApplicationState::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
        TriggerOnBackground();
  }
  last_state_ = state;
}


bool BackgroundHelperAndroid::IsForeground() const {
  bool foreground = (base::android::ApplicationStatusListener::GetState() ==
      base::android::ApplicationState::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) ? 
      true : false;
  return foreground;
}

void BackgroundHelperAndroid::CheckState() {
  if (IsForeground()) {
    TriggerOnForeground();
  } else {
    TriggerOnBackground();
  }
}


BackgroundHelperAndroid* BackgroundHelperAndroid::GetInstance() {
  return base::Singleton<BackgroundHelperAndroid>::get();
}

BackgroundHelper* BackgroundHelper::GetInstance() {
  return BackgroundHelperAndroid::GetInstance();
}


}  // namespace brave_ads
