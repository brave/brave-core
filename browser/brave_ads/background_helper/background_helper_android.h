/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_ANDROID_H_
#define BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_ANDROID_H_

#include <memory>

#include "base/android/application_status_listener.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_ads/background_helper/background_helper.h"

namespace brave_ads {

class BackgroundHelperAndroid
    : public BackgroundHelper,
      public base::SupportsWeakPtr<BackgroundHelperAndroid> {
 public:
  ~BackgroundHelperAndroid() override;

  BackgroundHelperAndroid(const BackgroundHelperAndroid&) = delete;
  BackgroundHelperAndroid& operator=(const BackgroundHelperAndroid&) = delete;

 protected:
  friend class BackgroundHelperHolder;

  BackgroundHelperAndroid();

 private:
  void OnApplicationStateChange(base::android::ApplicationState state);

  // BackgroundHelper impl
  bool IsForeground() const override;

  std::unique_ptr<base::android::ApplicationStatusListener>
      app_status_listener_;

  base::android::ApplicationState last_state_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_ANDROID_H_
