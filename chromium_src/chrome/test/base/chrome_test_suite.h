/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_SUITE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_SUITE_H_

#include "brave/components/brave_shields/content/browser/brave_shields_test_utils.h"

#define ChromeTestSuite ChromeTestSuite_ChromiumImpl
#include "src/chrome/test/base/chrome_test_suite.h"  // IWYU pragma: export
#undef ChromeTestSuite

class ChromeTestSuite : public ChromeTestSuite_ChromiumImpl {
 public:
  ChromeTestSuite(int argc, char** argv);
  ~ChromeTestSuite() override;

 protected:
  // base::TestSuite overrides:
  void Initialize() override;

 private:
  // Use stable farbling both in Brave and Chromium tests.
  brave_shields::ScopedStableFarblingTokensForTesting
      scoped_stable_farbling_tokens_{1};
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_SUITE_H_
