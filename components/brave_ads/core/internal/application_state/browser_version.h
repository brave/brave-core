/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_VERSION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_VERSION_H_

#include <string>

namespace brave_ads {

// Provides the current browser version number. Tests may inject a controlled
// value via SetForTesting to decouple version-sensitive logic from the real
// browser binary version.
class BrowserVersion {
 public:
  static const BrowserVersion& GetInstance();

  static void SetForTesting(const BrowserVersion* browser_version);

  BrowserVersion();

  BrowserVersion(const BrowserVersion&) = delete;
  BrowserVersion& operator=(const BrowserVersion&) = delete;

  virtual ~BrowserVersion();

  virtual std::string GetNumber() const;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_VERSION_H_
