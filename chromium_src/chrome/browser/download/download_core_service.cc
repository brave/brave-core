/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile.h"

namespace {
static int NonMaliciousDownloadCountTorProfile(Profile* profile);
}  // namespace

#include "../../../../../chrome/browser/download/download_core_service.cc"  // NOLINT

namespace {

// static
int NonMaliciousDownloadCountTorProfile(Profile* profile) {
  int count = 0;
  if (profile->HasTorProfile())
    count += DownloadCoreServiceFactory::GetForBrowserContext(
        profile->GetTorProfile())->NonMaliciousDownloadCount();
  return count;
}

}  // namespace
