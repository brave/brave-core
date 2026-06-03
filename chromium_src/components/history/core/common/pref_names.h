/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_COMMON_PREF_NAMES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_COMMON_PREF_NAMES_H_

#include <components/history/core/common/pref_names.h>  // IWYU pragma: export

namespace prefs {

// Number of days to retain browsing history. -1 keeps history forever.
inline constexpr char kBraveHistoryRetentionDays[] =
    "brave.history.retention_days";

}  // namespace prefs

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_COMMON_PREF_NAMES_H_
