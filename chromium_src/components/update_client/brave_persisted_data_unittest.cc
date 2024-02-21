/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/widevine/static_buildflags.h"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

#include "base/test/task_environment.h"
#include "components/prefs/testing_pref_service.h"
#include "components/update_client/persisted_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace update_client {

TEST(BravePersistedDataTest, UpstreamHasArm64Widevine) {
  base::test::TaskEnvironment env;
  auto pref = std::make_unique<TestingPrefServiceSimple>();
  RegisterPersistedDataPrefs(pref->registry());
  EXPECT_FALSE(UpstreamHasArm64Widevine(pref.get()));
  SetUpstreamHasArm64Widevine(pref.get());
  EXPECT_TRUE(UpstreamHasArm64Widevine(pref.get()));
}

}  // namespace update_client

#endif  // BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
