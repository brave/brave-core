/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_pref_info.h"

#include <utility>

namespace brave_origin {

BraveOriginPrefInfo::BraveOriginPrefInfo() = default;

BraveOriginPrefInfo::BraveOriginPrefInfo(const std::string& pref_name,
                                         base::Value default_value,
                                         BraveOriginPolicyScope scope,
                                         bool user_settable,
                                         const std::string& policy_key)
    : pref_name(pref_name),
      default_value(std::move(default_value)),
      scope(scope),
      user_settable(user_settable),
      policy_key(policy_key) {}

BraveOriginPrefInfo::~BraveOriginPrefInfo() = default;

BraveOriginPrefInfo::BraveOriginPrefInfo(BraveOriginPrefInfo&&) = default;

BraveOriginPrefInfo& BraveOriginPrefInfo::operator=(BraveOriginPrefInfo&&) =
    default;

BraveOriginPrefMetadata::BraveOriginPrefMetadata(
    base::Value origin_default_value,
    BraveOriginPolicyScope scope,
    bool user_settable)
    : origin_default_value(std::move(origin_default_value)),
      scope(scope),
      user_settable(user_settable) {}

BraveOriginPrefMetadata::~BraveOriginPrefMetadata() = default;

BraveOriginPrefMetadata::BraveOriginPrefMetadata(BraveOriginPrefMetadata&&) =
    default;

BraveOriginPrefMetadata& BraveOriginPrefMetadata::operator=(
    BraveOriginPrefMetadata&&) = default;

const BraveOriginPrefInfo* GetPrefInfo(
    const BraveOriginPrefMap& pref_definitions,
    const std::string& pref_name) {
  auto it = pref_definitions.find(pref_name);
  if (it != pref_definitions.end()) {
    return &it->second;
  }
  return nullptr;
}

}  // namespace brave_origin
