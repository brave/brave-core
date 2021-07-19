/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_TYPES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_TYPES_H_

// clang-format off
#define BRAVE_CONTENT_SETTINGS_TYPES_LIST                                      \
  BRAVE_ADS,                                                                   \
  BRAVE_COSMETIC_FILTERING,                                                    \
  BRAVE_TRACKERS,                                                              \
  BRAVE_HTTP_UPGRADABLE_RESOURCES,                                             \
  BRAVE_FINGERPRINTING_V2,                                                     \
  BRAVE_SHIELDS,                                                               \
  BRAVE_REFERRERS,                                                             \
  BRAVE_COOKIES,                                                               \
  BRAVE_SPEEDREADER,                                                           \
  BRAVE_ETHEREUM
// clang-format on

#include "../../../../../../components/content_settings/core/common/content_settings_types.h"

#undef BRAVE_CONTENT_SETTINGS_TYPES_LIST

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_TYPES_H_
