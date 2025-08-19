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
                      const std::string& policy_key);
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
};

using BraveOriginPrefMap = base::flat_map<std::string, BraveOriginPrefInfo>;

// Static BraveOrigin-specific metadata for policy preferences.
// This defines which preferences from kBraveSimplePolicyMap should have
// BraveOrigin behavior and specifies their BraveOrigin-specific configuration
// (default values, scope, UI visibility). Used only during initialization
// to populate BraveOriginPrefInfo structs.
struct BraveOriginPrefMetadata {
  BraveOriginPrefMetadata(base::Value origin_default_value,
                          BraveOriginPolicyScope scope,
                          bool user_settable);
  ~BraveOriginPrefMetadata();

  // Copy operations are deleted because base::Value is not copyable
  BraveOriginPrefMetadata(const BraveOriginPrefMetadata&) = delete;
  BraveOriginPrefMetadata& operator=(const BraveOriginPrefMetadata&) = delete;

  BraveOriginPrefMetadata(BraveOriginPrefMetadata&&);
  BraveOriginPrefMetadata& operator=(BraveOriginPrefMetadata&&);

  base::Value origin_default_value;
  BraveOriginPolicyScope scope;
  bool user_settable;
};

// Helper function to get pref info from pref definitions
const BraveOriginPrefInfo* GetPrefInfo(
    const BraveOriginPrefMap& pref_definitions,
    const std::string& pref_name);

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREF_INFO_H_
