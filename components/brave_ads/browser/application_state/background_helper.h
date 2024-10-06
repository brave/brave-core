/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_H_

#include "base/observer_list.h"

namespace brave_ads {

class BackgroundHelper {
 public:
  class Observer {
   public:
    virtual ~Observer() = default;

    virtual void OnBrowserDidEnterForeground() = 0;
    virtual void OnBrowserDidEnterBackground() = 0;
  };

  BackgroundHelper(const BackgroundHelper&) = delete;
  BackgroundHelper& operator=(const BackgroundHelper&) = delete;

  BackgroundHelper(BackgroundHelper&&) noexcept = delete;
  BackgroundHelper& operator=(BackgroundHelper&&) noexcept = delete;

  virtual ~BackgroundHelper();

  static BackgroundHelper* GetInstance();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void TriggerOnForeground();
  void TriggerOnBackground();

  virtual bool IsForeground() const;

 protected:
  friend class BackgroundHelperHolder;

  BackgroundHelper();

 private:
  base::ObserverList<Observer>::Unchecked observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_BACKGROUND_HELPER_H_
