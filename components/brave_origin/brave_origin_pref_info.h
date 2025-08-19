/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREF_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREF_INFO_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/values.h"

namespace brave_origin {

enum class BraveOriginPolicyScope {
  kGlobal,  // Stored in local state
  kProfile  // Stored in profile prefs
};

// Complete runtime information for a BraveOrigin-controlled preference.
// This combines data from kBraveSimplePolicyMap (pref_name, policy_key) with
// BraveOrigin-specific metadata (default_value, scope, user_settable) to create
// the final preference definition used throughout the system.
struct BraveOriginPrefInfo {
  BraveOriginPrefInfo();
  BraveOriginPrefInfo(const std::string& pref_name,
                      base::Value default_value,
                      BraveOriginPolicyScope scope,
                      bool user_settable,
                      const std::string& policy_key,
                      const std::string& brave_origin_pref_key);
  ~BraveOriginPrefInfo();

  // Copy operations are deleted because base::Value is not copyable
  BraveOriginPrefInfo(const BraveOriginPrefInfo&) = delete;
  BraveOriginPrefInfo& operator=(const BraveOriginPrefInfo&) = delete;

  BraveOriginPrefInfo(BraveOriginPrefInfo&&);
  BraveOriginPrefInfo& operator=(BraveOriginPrefInfo&&);

  std::string pref_name;
  base::Value default_value;
  BraveOriginPolicyScope scope;
  bool user_settable;      // Whether this pref has UI for user control
  std::string policy_key;  // Policy key that controls this pref
  std::string brave_origin_pref_key;  // Key used in brave_policies dictionary
};

using BraveOriginPrefMap = base::flat_map<std::string, BraveOriginPrefInfo>;

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREF_INFO_H_
