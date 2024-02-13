/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/android/hats/hats_service_android.h"

#define HatsServiceAndroid HatsServiceAndroid_ChromiumImpl
#include "src/chrome/browser/ui/android/hats/hats_service_android.cc"
#undef HatsServiceAndroid

HatsServiceAndroid::HatsServiceAndroid(Profile* profile)
    : HatsServiceAndroid_ChromiumImpl(profile) {}

HatsServiceAndroid::~HatsServiceAndroid() = default;

bool HatsServiceAndroid::CanShowSurvey(const std::string& trigger) const {
  return false;
}
