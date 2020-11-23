/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/widevine/cdm/buildflags.h"

// We don't bundle. Only use widevine as a component.
TEST(WidevineBuildFlag, FlagTest) {
  EXPECT_TRUE(
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
      false);
#else
      true);
#endif

  EXPECT_TRUE(
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
      true);
#else
      false);
#endif
}
