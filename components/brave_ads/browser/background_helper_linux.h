/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_LINUX_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_LINUX_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/browser/background_helper.h"
#include "chrome/browser/ui/browser_list_observer.h"

namespace brave_ads {

class BackgroundHelperLinux :
    public BackgroundHelper,
    public base::SupportsWeakPtr<BackgroundHelperLinux>,
    public BrowserListObserver {
 public:
  BackgroundHelperLinux();
  ~BackgroundHelperLinux() override;

  static BackgroundHelperLinux* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BackgroundHelperLinux>;

  // BackgroundHelper impl
  bool IsForeground() const override;

  // BrowserListObserver overrides
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  void CheckState();

  DISALLOW_COPY_AND_ASSIGN(BackgroundHelperLinux);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BACKGROUND_HELPER_LINUX_H_
