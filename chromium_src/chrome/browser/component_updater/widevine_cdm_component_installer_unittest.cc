/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/widevine/cdm/buildflags.h"

// This test looks a bit strange but it will protect us against if Chromium changes
// the build flags for Widevine.
TEST(WidevineBuildFlag, MustBeEnabledForNonLinux) {
  ASSERT_TRUE(
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
      true
#else
      false
#endif
  );
}
