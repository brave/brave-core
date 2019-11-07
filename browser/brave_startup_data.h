/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STARTUP_DATA_H_
#define BRAVE_BROWSER_BRAVE_STARTUP_DATA_H_

#include "chrome/browser/startup_data.h"

class BraveStartupData : public StartupData {
 public:
  BraveStartupData() = default;
  ~BraveStartupData() override = default;

 private:
#if defined(OS_ANDROID)
  void PreProfilePrefServiceInit() override;
#endif

  DISALLOW_COPY_AND_ASSIGN(BraveStartupData);
};

#endif  // BRAVE_BROWSER_BRAVE_STARTUP_DATA_H_
