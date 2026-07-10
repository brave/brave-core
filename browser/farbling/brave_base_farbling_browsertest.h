// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_FARBLING_BRAVE_BASE_FARBLING_BROWSERTEST_H_
#define BRAVE_BROWSER_FARBLING_BRAVE_BASE_FARBLING_BROWSERTEST_H_

#include "chrome/test/base/in_process_browser_test.h"

// Base class for farbling browser tests. Zeros the profile-level farbling
// entropy in SetUpOnMainThread so PRNG golden values remain stable across
// browser sessions while still exercising the kBraveFarblingTokenReset
// feature-enabled code path (zero XOR is a no-op).
class BraveBaseFarblingBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override;
};

#endif  // BRAVE_BROWSER_FARBLING_BRAVE_BASE_FARBLING_BROWSERTEST_H_
