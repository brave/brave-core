/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_MANAGER_BROWSER_MANAGER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_MANAGER_BROWSER_MANAGER_OBSERVER_H_

#include "base/observer_list_types.h"

namespace ads {

class BrowserManagerObserver : public base::CheckedObserver {
 public:
  // Tells the delegate that the browser has become active.
  virtual void OnBrowserDidBecomeActive() {}

  // Tells the delegate that the browser did become inactive.
  virtual void OnBrowserDidResignActive() {}

  // Tells the delegate that the browser is now in the background.
  virtual void OnBrowserDidEnterForeground() {}

  // Tells the delegate that the browser is now in the foreground.
  virtual void OnBrowserDidEnterBackground() {}

 protected:
  ~BrowserManagerObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_MANAGER_BROWSER_MANAGER_OBSERVER_H_
