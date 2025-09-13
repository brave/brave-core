/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_service.h"

namespace brave_origin {

BraveOriginService::BraveOriginService(const std::string& profile_id)
    : profile_id_(profile_id) {}

BraveOriginService::~BraveOriginService() = default;

bool BraveOriginService::IsPrefControlledByBraveOrigin(
    const std::string& pref_name) const {
  // Check if a Brave Origin policy controls the pref
  return false;
}

bool BraveOriginService::SetBraveOriginPolicyValue(const std::string& pref_name,
                                                   base::Value value) {
  // Set a Brave Origin policy value
  return false;
}

base::Value BraveOriginService::GetBraveOriginPrefValue(
    const std::string& pref_name) const {
  // Get a Brave Origin policy value
  return base::Value();
}

}  // namespace brave_origin
