/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"

#define FromSeconds \
  Max().is_max() ? base::Seconds(time_to_live_seconds) : base::Seconds
#define FALLTHROUGH [[fallthrough]]
#include "src/chrome/browser/extensions/api/identity/identity_get_auth_token_function.cc"
#undef FALLTHROUGH
#undef FromSeconds
