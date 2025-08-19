/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/grit/brave_components_strings.h"
#include "components/policy/core/common/policy_types.h"
#include "components/strings/grit/components_strings.h"

// Capture the original number
enum {
  kBravePolicySourceEnterpriseDefault = IDS_POLICY_SOURCE_ENTERPRISE_DEFAULT
};

// Override macro: inject your Brave entry right after EnterpriseDefault
#undef IDS_POLICY_SOURCE_ENTERPRISE_DEFAULT
#define IDS_POLICY_SOURCE_ENTERPRISE_DEFAULT \
  kBravePolicySourceEnterpriseDefault        \
  }                                          \
  , {                                        \
    "braveOrigin", IDS_POLICY_SOURCE_BRAVE_ORIGIN

#include <components/policy/core/browser/policy_conversions.cc>  // IWYU pragma: export

#undef IDS_POLICY_SOURCE_ENTERPRISE_DEFAULT

// ************
// ************
// TODO add a test to make sure it appears in the array
// ************
// ************
