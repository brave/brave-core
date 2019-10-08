/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_ads/browser/background_helper_android.h"

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace brave_ads {

BackgroundHelperAndroid::BackgroundHelperAndroid() {
  app_status_listener_ = base::android::ApplicationStatusListener::New(
      base::BindRepeating(&BackgroundHelperAndroid::OnApplicationStateChange,
      AsWeakPtr()));

  last_state_ = base::android::ApplicationStatusListener::GetState();
}

BackgroundHelperAndroid::~BackgroundHelperAndroid() {
  app_status_listener_.reset();
}

bool BackgroundHelperAndroid::IsForeground() const {
  auto state = base::android::ApplicationStatusListener::GetState();
  if (state != base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    return false;
  }

  return true;
}

void BackgroundHelperAndroid::OnApplicationStateChange(
    base::android::ApplicationState state) {
  if (state == base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    TriggerOnForeground();
  } else if (last_state_ != state && last_state_ ==
      base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    TriggerOnBackground();
  }

  last_state_ = state;
}

BackgroundHelperAndroid* BackgroundHelperAndroid::GetInstance() {
  return base::Singleton<BackgroundHelperAndroid>::get();
}

BackgroundHelper* BackgroundHelper::GetInstance() {
  return BackgroundHelperAndroid::GetInstance();
}


}  // namespace brave_ads
