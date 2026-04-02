/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_H_

#include "base/observer_list.h"
#include "brave/components/brave_ads/browser/application_state/background_helper_observer.h"

namespace brave_ads {

// Notifies `BackgroundHelperObserver` when the browser becomes active or
// inactive.
class BackgroundHelper {
 public:
  BackgroundHelper();

  BackgroundHelper(const BackgroundHelper&) = delete;
  BackgroundHelper& operator=(const BackgroundHelper&) = delete;

  virtual ~BackgroundHelper();

  // Returns the singleton instance.
  static BackgroundHelper* GetInstance();

  void AddObserver(BackgroundHelperObserver* observer);
  void RemoveObserver(BackgroundHelperObserver* observer);

  // Returns whether the browser is currently in the foreground.
  virtual bool IsInForeground() const;

  // Notifies observers that the browser entered the foreground.
  void NotifyDidEnterForeground();

  // Notifies observers that the browser entered the background.
  void NotifyDidEnterBackground();

 private:
  base::ObserverList<BackgroundHelperObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_H_
