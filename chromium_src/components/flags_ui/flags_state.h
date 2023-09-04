/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_FLAGS_UI_FLAGS_STATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_FLAGS_UI_FLAGS_STATE_H_

#define GetFlagFeatureEntries(...)                                             \
  GetFlagFeatureEntries(__VA_ARGS__);                                          \
  base::Value::List CreateOptionsData(                                         \
      const FeatureEntry& entry, const std::set<std::string>& enabled_entries) \
      const

#include "src/components/flags_ui/flags_state.h"  // IWYU pragma: export

#undef GetFlagFeatureEntries

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_FLAGS_UI_FLAGS_STATE_H_
