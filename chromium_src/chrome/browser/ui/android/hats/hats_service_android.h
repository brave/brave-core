/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_ANDROID_HATS_HATS_SERVICE_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_ANDROID_HATS_HATS_SERVICE_ANDROID_H_

#define HatsServiceAndroid HatsServiceAndroid_ChromiumImpl
#include "src/chrome/browser/ui/android/hats/hats_service_android.h"  // IWYU pragma: export
#undef HatsServiceAndroid

class HatsServiceAndroid : public HatsServiceAndroid_ChromiumImpl {
 public:
  explicit HatsServiceAndroid(Profile* profile);
  HatsServiceAndroid(const HatsServiceAndroid&) = delete;
  HatsServiceAndroid& operator=(const HatsServiceAndroid&) = delete;

  ~HatsServiceAndroid() override;

  bool CanShowSurvey(const std::string& trigger) const override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_ANDROID_HATS_HATS_SERVICE_ANDROID_H_
