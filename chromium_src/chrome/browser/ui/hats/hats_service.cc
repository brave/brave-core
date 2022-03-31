/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/hats/hats_service.h"

#define HatsService HatsService_ChromiumImpl
#include "src/chrome/browser/ui/hats/hats_service.cc"
#undef HatsService

HatsService::HatsService(Profile* profile)
    : HatsService_ChromiumImpl(profile) {}

HatsService::~HatsService() = default;

bool HatsService::CanShowSurvey(const std::string& trigger) const {
  return false;
}
