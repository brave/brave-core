/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/test/base/testing_profile_manager.cc"

void TestingProfileManager::SetProfileAsLastUsed(Profile* last_active) {
#if !BUILDFLAG(IS_ANDROID)
  profile_manager_->SetProfileAsLastUsed(last_active);
#endif
}
