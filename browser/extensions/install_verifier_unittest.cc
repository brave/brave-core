/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/install_verifier.h"

#include "testing/gtest/include/gtest/gtest.h"

#if defined(OFFICIAL_BUILD)

#if defined(OS_WIN) || defined(OS_MAC)
TEST(InstallVerifierUnitTest, TestShouldEnforce) {
  EXPECT_TRUE(extensions::InstallVerifier::ShouldEnforce());
}
#else
TEST(InstallVerifierUnitTest, TestShouldNotEnforce) {
  EXPECT_FALSE(extensions::InstallVerifier::ShouldEnforce());
}
#endif  // defined(OS_WIN) || defined(OS_MAC)

#endif  // defined(OFFICIAL_BUILD)