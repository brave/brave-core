/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_policy_info.h"

#include <utility>

namespace brave_origin {

BraveOriginPolicyInfo::BraveOriginPolicyInfo() = default;

BraveOriginPolicyInfo::BraveOriginPolicyInfo(
    const std::string& pref_name,
    bool default_value,
    bool user_settable,
    const std::string& brave_origin_pref_key)
    : pref_name(pref_name),
      default_value(default_value),
      user_settable(user_settable),
      brave_origin_pref_key(brave_origin_pref_key) {}

BraveOriginPolicyInfo::~BraveOriginPolicyInfo() = default;

BraveOriginPolicyInfo::BraveOriginPolicyInfo(BraveOriginPolicyInfo&&) = default;

BraveOriginPolicyInfo& BraveOriginPolicyInfo::operator=(
    BraveOriginPolicyInfo&&) = default;

}  // namespace brave_origin
