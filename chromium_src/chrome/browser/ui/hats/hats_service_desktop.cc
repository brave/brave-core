/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/hats/hats_service_desktop.h"

#define HatsServiceDesktop HatsServiceDesktop_ChromiumImpl
#include <chrome/browser/ui/hats/hats_service_desktop.cc>
#undef HatsServiceDesktop

HatsServiceDesktop::HatsServiceDesktop(Profile* profile)
    : HatsServiceDesktop_ChromiumImpl(profile) {}

HatsServiceDesktop::~HatsServiceDesktop() = default;

HatsService::LaunchError HatsServiceDesktop::GetCommonLaunchError(
    const std::string& trigger) const {
  // Brave never shows HaTS surveys. Returning a non-kNone error here aborts the
  // launch path before a dialog is created and makes `CanShowSurvey()` return
  // false.
  return LaunchError::kError;
}
