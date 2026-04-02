/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_OBSERVER_H_

#include "base/observer_list_types.h"

namespace brave_ads {

class BackgroundHelperObserver : public base::CheckedObserver {
 public:
  ~BackgroundHelperObserver() override = default;

  // Called when the browser enters the foreground.
  virtual void OnBrowserDidEnterForeground() = 0;

  // Called when the browser enters the background.
  virtual void OnBrowserDidEnterBackground() = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_OBSERVER_H_
