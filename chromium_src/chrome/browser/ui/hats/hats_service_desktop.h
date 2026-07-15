/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_HATS_HATS_SERVICE_DESKTOP_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_HATS_HATS_SERVICE_DESKTOP_H_

#define HatsServiceDesktop HatsServiceDesktop_ChromiumImpl

// Brave disables HaTS surveys entirely. Upstream funnels every survey-show
// decision through the private, non-virtual `GetCommonLaunchError()`: both the
// public `CanShowSurvey()` and the internal launch path
// (`ShowSurvey()` -> `GetLaunchError()` -> `GetCommonLaunchError()`) call it.
// Make it virtual so we can override it as the single chokepoint that blocks
// all surveys.
#define GetCommonLaunchError     \
  GetCommonLaunchError_Unused(); \
  virtual LaunchError GetCommonLaunchError

#include <chrome/browser/ui/hats/hats_service_desktop.h>  // IWYU pragma: export
#undef GetCommonLaunchError
#undef HatsServiceDesktop

class HatsServiceDesktop : public HatsServiceDesktop_ChromiumImpl {
 public:
  explicit HatsServiceDesktop(Profile* profile);
  HatsServiceDesktop(const HatsServiceDesktop&) = delete;
  HatsServiceDesktop& operator=(const HatsServiceDesktop&) = delete;

  ~HatsServiceDesktop() override;

 private:
  LaunchError GetCommonLaunchError(const std::string& trigger) const override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_HATS_HATS_SERVICE_DESKTOP_H_
