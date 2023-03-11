/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PREFS_PREF_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PREFS_PREF_SERVICE_H_

#define GetBoolean                                         \
  GetBooleanOr(const std::string& path, bool other) const; \
  bool GetBoolean

#include "src/components/prefs/pref_service.h"  // IWYU pragma: export

#undef GetBoolean

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PREFS_PREF_SERVICE_H_
