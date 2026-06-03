/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_TEST_FAKE_BROWSER_VERSION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_TEST_FAKE_BROWSER_VERSION_H_

#include <string>

#include "brave/components/brave_ads/core/internal/application_state/browser_version.h"

namespace brave_ads::test {

inline constexpr char kDefaultBrowserVersionNumber[] = "1.2.3.4";

// A test double for `BrowserVersion` that installs itself as the active
// instance on construction and restores the default on destruction. Call
// `SetNumber` to change the simulated version at any point during a test.
class FakeBrowserVersion final : public BrowserVersion {
 public:
  FakeBrowserVersion();

  FakeBrowserVersion(const FakeBrowserVersion&) = delete;
  FakeBrowserVersion& operator=(const FakeBrowserVersion&) = delete;

  ~FakeBrowserVersion() override;

  void SetNumber(std::string number);

  // BrowserVersion:
  std::string GetNumber() const override;

 private:
  std::string number_ = kDefaultBrowserVersionNumber;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_TEST_FAKE_BROWSER_VERSION_H_
