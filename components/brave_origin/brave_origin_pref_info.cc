/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_pref_info.h"

#include <utility>

namespace brave_origin {

BraveOriginPrefInfo::BraveOriginPrefInfo() = default;

BraveOriginPrefInfo::BraveOriginPrefInfo(
    const std::string& pref_name,
    base::Value default_value,
    BraveOriginPolicyScope scope,
    bool user_settable,
    const std::string& policy_key,
    const std::string& brave_origin_pref_key)
    : pref_name(pref_name),
      default_value(std::move(default_value)),
      scope(scope),
      user_settable(user_settable),
      policy_key(policy_key),
      brave_origin_pref_key(brave_origin_pref_key) {}

BraveOriginPrefInfo::~BraveOriginPrefInfo() = default;

BraveOriginPrefInfo::BraveOriginPrefInfo(BraveOriginPrefInfo&&) = default;

BraveOriginPrefInfo& BraveOriginPrefInfo::operator=(BraveOriginPrefInfo&&) =
    default;

}  // namespace brave_origin
