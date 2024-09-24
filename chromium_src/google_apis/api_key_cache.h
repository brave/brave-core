/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_GOOGLE_APIS_API_KEY_CACHE_H_
#define BRAVE_CHROMIUM_SRC_GOOGLE_APIS_API_KEY_CACHE_H_

#define metrics_key                                          \
  Undefined();                                               \
  void set_api_key_for_testing(const std::string& api_key) { \
    api_key_ = api_key;                                      \
  }                                                          \
  const std::string& metrics_key

#include "src/google_apis/api_key_cache.h"  // IWYU pragma: export

#undef metrics_key

#endif  // BRAVE_CHROMIUM_SRC_GOOGLE_APIS_API_KEY_CACHE_H_
