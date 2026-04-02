/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/background_helper/background_helper_android.h"

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"

namespace brave_ads {

// static
BackgroundHelper* BackgroundHelper::GetInstance() {
  static base::NoDestructor<BackgroundHelperAndroid> instance;
  return instance.get();
}

BackgroundHelperAndroid::BackgroundHelperAndroid() {
  application_status_listener_ = base::android::ApplicationStatusListener::New(
      base::BindRepeating(&BackgroundHelperAndroid::OnApplicationStateChange,
                          weak_ptr_factory_.GetWeakPtr()));

  application_state_ = base::android::ApplicationStatusListener::GetState();
}

BackgroundHelperAndroid::~BackgroundHelperAndroid() = default;

bool BackgroundHelperAndroid::IsInForeground() const {
  return base::android::ApplicationStatusListener::GetState() ==
         base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES;
}

void BackgroundHelperAndroid::OnApplicationStateChange(
    base::android::ApplicationState state) {
  if (state == base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    NotifyDidEnterForeground();
  } else if (application_state_ != state &&
             application_state_ ==
                 base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    NotifyDidEnterBackground();
  }

  application_state_ = state;
}

}  // namespace brave_ads
