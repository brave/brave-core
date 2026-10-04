/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_INFO_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/values.h"

namespace brave_origin {

// Complete runtime information for a BraveOrigin-controlled preference.
// This combines data from kBraveSimplePolicyMap (pref_name, policy_key) with
// BraveOrigin-specific metadata (default_value, scope, user_settable) to create
// the final preference definition used throughout the system.
struct BraveOriginPolicyInfo {
  BraveOriginPolicyInfo();
  BraveOriginPolicyInfo(const std::string& pref_name,
                        bool default_value,
                        bool user_settable,
                        const std::string& brave_origin_pref_key);
  ~BraveOriginPolicyInfo();

  // Copy operations are deleted to enforce move semantics
  BraveOriginPolicyInfo(const BraveOriginPolicyInfo&) = delete;
  BraveOriginPolicyInfo& operator=(const BraveOriginPolicyInfo&) = delete;

  BraveOriginPolicyInfo(BraveOriginPolicyInfo&&);
  BraveOriginPolicyInfo& operator=(BraveOriginPolicyInfo&&);

  std::string pref_name;
  bool default_value;
  bool user_settable;  // Whether this pref has UI for user control
  std::string brave_origin_pref_key;  // Key used in brave_policies dictionary
};

using BraveOriginPolicyMap = base::flat_map<std::string, BraveOriginPolicyInfo>;

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_POLICY_INFO_H_
