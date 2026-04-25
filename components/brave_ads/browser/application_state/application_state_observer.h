/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_APPLICATION_STATE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_APPLICATION_STATE_OBSERVER_H_

#include "base/observer_list_types.h"

namespace brave_ads {

class ApplicationStateObserver : public base::CheckedObserver {
 public:
  ~ApplicationStateObserver() override = default;

  // Called when the browser becomes active.
  virtual void OnBrowserDidBecomeActive() = 0;

  // Called when the browser resigns active.
  virtual void OnBrowserDidResignActive() = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_APPLICATION_STATE_OBSERVER_H_
