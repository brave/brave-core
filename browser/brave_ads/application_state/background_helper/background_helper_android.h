/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_ANDROID_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_ANDROID_H_

#include <memory>

#include "base/android/application_status_listener.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/browser/application_state/background_helper.h"

namespace brave_ads {

class BackgroundHelperAndroid final : public BackgroundHelper {
 public:
  BackgroundHelperAndroid(const BackgroundHelperAndroid&) = delete;
  BackgroundHelperAndroid& operator=(const BackgroundHelperAndroid&) = delete;

  BackgroundHelperAndroid(BackgroundHelperAndroid&&) noexcept = delete;
  BackgroundHelperAndroid& operator=(BackgroundHelperAndroid&&) noexcept =
      delete;

  ~BackgroundHelperAndroid() override;

 protected:
  friend class BackgroundHelperHolder;

  BackgroundHelperAndroid();

 private:
  void OnApplicationStateChange(base::android::ApplicationState state);

  // BackgroundHelper:
  bool IsForeground() const override;

  std::unique_ptr<base::android::ApplicationStatusListener>
      app_status_listener_;

  base::android::ApplicationState last_state_;

  base::WeakPtrFactory<BackgroundHelperAndroid> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_BACKGROUND_HELPER_BACKGROUND_HELPER_ANDROID_H_
