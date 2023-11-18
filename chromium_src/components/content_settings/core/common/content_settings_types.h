/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_TYPES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_TYPES_H_

#define NUM_TYPES                                                              \
  BRAVE_START, BRAVE_ADS = BRAVE_START, BRAVE_COSMETIC_FILTERING,              \
               BRAVE_TRACKERS, BRAVE_HTTP_UPGRADABLE_RESOURCES,                \
               BRAVE_FINGERPRINTING_V2, BRAVE_SHIELDS, BRAVE_REFERRERS,        \
               BRAVE_COOKIES, BRAVE_SPEEDREADER, BRAVE_ETHEREUM, BRAVE_SOLANA, \
               BRAVE_GOOGLE_SIGN_IN, BRAVE_HTTPS_UPGRADE,                      \
               BRAVE_REMEMBER_1P_STORAGE, BRAVE_LOCALHOST_ACCESS, NUM_TYPES

#include "src/components/content_settings/core/common/content_settings_types.h"  // IWYU pragma: export

#undef NUM_TYPES

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_TYPES_H_
