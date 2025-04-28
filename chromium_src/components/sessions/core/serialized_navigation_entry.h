/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_

#define set_http_status_code(...)                                   \
  NotUsed() const;                                                  \
  std::map<std::string, std::string>& mutable_extended_info_map() { \
    return extended_info_map_;                                      \
  }                                                                 \
  void set_http_status_code(__VA_ARGS__)

#include "src/components/sessions/core/serialized_navigation_entry.h"  // IWYU pragma: export

#undef set_http_status_code

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_
