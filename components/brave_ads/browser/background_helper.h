/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "build/build_config.h"

namespace brave_ads {

class BackgroundHelper {
 public:
  class Observer {
   public:
    virtual void OnBackground() = 0;
    virtual void OnForeground() = 0;
  };

  static BackgroundHelper* GetInstance();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void TriggerOnBackground();
  void TriggerOnForeground();

  virtual bool IsForeground() const;

 protected:
  BackgroundHelper();
  virtual ~BackgroundHelper();

 private:
  base::ObserverList<Observer>::Unchecked observers_;

  friend struct base::DefaultSingletonTraits<BackgroundHelper>;
  DISALLOW_COPY_AND_ASSIGN(BackgroundHelper);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_H_
