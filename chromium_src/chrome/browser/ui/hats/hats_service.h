/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_HATS_HATS_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_HATS_HATS_SERVICE_H_

#define HatsService HatsService_ChromiumImpl
#define CanShowSurvey virtual CanShowSurvey
#include "src/chrome/browser/ui/hats/hats_service.h"
#undef CanShowSurvey
#undef HatsService

class HatsService : public HatsService_ChromiumImpl {
 public:
  explicit HatsService(Profile* profile);
  HatsService(const HatsService&) = delete;
  HatsService& operator=(const HatsService&) = delete;

  ~HatsService() override;

  bool CanShowSurvey(const std::string& trigger) const override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_HATS_HATS_SERVICE_H_
