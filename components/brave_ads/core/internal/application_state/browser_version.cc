/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_version.h"

#include "base/check_is_test.h"
#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"
#include "brave/components/version_info/version_info.h"

namespace brave_ads {

namespace {

const BrowserVersion* g_browser_version_for_testing = nullptr;

}  // namespace

// static
const BrowserVersion& BrowserVersion::GetInstance() {
  if (g_browser_version_for_testing) {
    CHECK_IS_TEST();

    return *g_browser_version_for_testing;
  }

  static const base::NoDestructor<BrowserVersion> kBrowserVersion;
  return *kBrowserVersion;
}

// static
void BrowserVersion::SetForTesting(  // IN-TEST
    const BrowserVersion* const browser_version) {
  CHECK_IS_TEST();

  g_browser_version_for_testing = browser_version;
  ResetBrowserUpgradeCacheForTesting();  // IN-TEST
}

BrowserVersion::BrowserVersion() = default;

BrowserVersion::~BrowserVersion() = default;

std::string BrowserVersion::GetNumber() const {
  return version_info::GetBraveChromiumVersionNumber();
}

}  // namespace brave_ads
