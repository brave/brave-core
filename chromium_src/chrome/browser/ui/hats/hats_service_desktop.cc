/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/hats/hats_service_desktop.h"

#define HatsServiceDesktop HatsServiceDesktop_ChromiumImpl
#include "src/chrome/browser/ui/hats/hats_service_desktop.cc"
#undef HatsServiceDesktop

HatsServiceDesktop::HatsServiceDesktop(Profile* profile)
    : HatsServiceDesktop_ChromiumImpl(profile) {}

HatsServiceDesktop::~HatsServiceDesktop() = default;

bool HatsServiceDesktop::CanShowSurvey(const std::string& trigger) const {
  return false;
}
