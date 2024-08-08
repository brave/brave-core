/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_TESTING_PROFILE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_TESTING_PROFILE_MANAGER_H_

// Reinstating `SetProfileAsLastUsed` as it has been deleted upstream, however
// this function is still in use in by `BraveWhatsNewTest`.
#define DeleteProfileAttributesStorage        \
  SetProfileAsLastUsed(Profile* last_active); \
  void DeleteProfileAttributesStorage

#include "src/chrome/test/base/testing_profile_manager.h"  // IWYU pragma: export

#undef DeleteProfileAttributesStorage

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_TESTING_PROFILE_MANAGER_H_
