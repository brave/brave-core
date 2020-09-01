/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/gaia_info_update_service.h"

#define ShouldUseGAIAProfileInfo ShouldUseGAIAProfileInfo_ChromiumImpl
#include "../../../../../chrome/browser/profiles/gaia_info_update_service.cc"
#undef ShouldUseGAIAProfileInfo

// static
bool GAIAInfoUpdateService::ShouldUseGAIAProfileInfo(Profile* profile) {
  return false;
}
