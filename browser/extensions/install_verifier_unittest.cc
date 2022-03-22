/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/install_verifier.h"

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OFFICIAL_BUILD)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
TEST(InstallVerifierUnitTest, TestShouldEnforce) {
  EXPECT_TRUE(extensions::InstallVerifier::ShouldEnforce());
}
#else
TEST(InstallVerifierUnitTest, TestShouldNotEnforce) {
  EXPECT_FALSE(extensions::InstallVerifier::ShouldEnforce());
}
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)

#endif  // defined(OFFICIAL_BUILD)
