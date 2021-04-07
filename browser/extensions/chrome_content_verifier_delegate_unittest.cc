/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/chrome_content_verifier_delegate.h"

#include "testing/gtest/include/gtest/gtest.h"

#if defined(OFFICIAL_BUILD)

TEST(ChromeContentVerifierDelegateUnitTest, TestShouldEnforce) {
  EXPECT_EQ(extensions::ChromeContentVerifierDelegate::GetDefaultMode(),
            extensions::ChromeContentVerifierDelegate::VerifyInfo::Mode::
                ENFORCE_STRICT);
}

#else

TEST(ChromeContentVerifierDelegateUnitTest, TestShouldNotEnforce) {
  EXPECT_EQ(extensions::ChromeContentVerifierDelegate::GetDefaultMode(),
            extensions::ChromeContentVerifierDelegate::VerifyInfo::Mode::NONE);
}

#endif  // defined(OFFICIAL_BUILD)