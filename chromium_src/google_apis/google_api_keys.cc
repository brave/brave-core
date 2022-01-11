/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/google_apis/google_api_keys.h"
#include "src/google_apis/google_api_keys.cc"

namespace google_apis {

void SetAPIKeyForTesting(const std::string& api_key) {
  g_api_key_cache.Get().set_api_key_for_testing(api_key);
}

}  // namespace google_apis
