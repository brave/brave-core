/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/policy/core/common/policy_types.h"

#define POLICY_SOURCE_MERGED                     \
  POLICY_SOURCE_BRAVE:                           \
  return em::Policy_PolicySource_SOURCE_UNKNOWN; \
  case policy::POLICY_SOURCE_MERGED

#include <components/enterprise/browser/reporting/policy_info.cc>  // IWYU pragma: export

#undef POLICY_SOURCE_MERGED
