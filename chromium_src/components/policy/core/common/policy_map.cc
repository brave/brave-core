/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Make sure the following override doesn't apply to this included file
#include "components/policy/core/common/policy_types.h"

#define POLICY_SOURCE_MERGED                          \
  POLICY_SOURCE_BRAVE_ORIGIN:                         \
  return PolicyPriorityBrowser::kBraveOriginPriority; \
  case POLICY_SOURCE_MERGED

#include <components/policy/core/common/policy_map.cc>  // IWYU pragma: export

#undef POLICY_SOURCE_MERGED

// *****************************
// *****************************
// *****************************
// TODO: Make UNIT TEST FOR THIS - because the default cause will call
// NOTREACHED() If ever this override doesn't take effect because of a chromium
// upgrade, we need tests to catch this
// *****************************
// *****************************
// *****************************
