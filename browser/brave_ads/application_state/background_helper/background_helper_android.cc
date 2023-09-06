/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/background_helper/background_helper_android.h"

#include "base/functional/bind.h"
#include "base/functional/callback.h"

namespace brave_ads {

BackgroundHelperAndroid::BackgroundHelperAndroid() {
  app_status_listener_ =
      base::android::ApplicationStatusListener::New(base::BindRepeating(
          &BackgroundHelperAndroid::OnApplicationStateChange, AsWeakPtr()));

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
  } else if (last_state_ != state &&
             last_state_ ==
                 base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    TriggerOnBackground();
  }

  last_state_ = state;
}

}  // namespace brave_ads
