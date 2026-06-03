/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/test/fake_browser_version.h"

#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"

namespace brave_ads::test {

FakeBrowserVersion::FakeBrowserVersion() {
  BrowserVersion::SetForTesting(this);
}

FakeBrowserVersion::~FakeBrowserVersion() {
  BrowserVersion::SetForTesting(nullptr);
}

void FakeBrowserVersion::SetNumber(std::string number) {
  number_ = std::move(number);
  ResetBrowserUpgradeCacheForTesting();
}

std::string FakeBrowserVersion::GetNumber() const {
  return number_;
}

}  // namespace brave_ads::test
