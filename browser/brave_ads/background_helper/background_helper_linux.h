/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_LINUX_H_
#define BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_LINUX_H_

#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_ads/background_helper/background_helper.h"
#include "chrome/browser/ui/browser_list_observer.h"

namespace brave_ads {

class BackgroundHelperLinux
    : public BackgroundHelper,
      public base::SupportsWeakPtr<BackgroundHelperLinux>,
      public BrowserListObserver {
 public:
  BackgroundHelperLinux(const BackgroundHelperLinux&) = delete;
  BackgroundHelperLinux& operator=(const BackgroundHelperLinux&) = delete;

  static BackgroundHelperLinux* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BackgroundHelperLinux>;

  BackgroundHelperLinux();
  ~BackgroundHelperLinux() override;

  // BrowserListObserver overrides
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  // BackgroundHelper impl
  bool IsForeground() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_BACKGROUND_HELPER_LINUX_H_
