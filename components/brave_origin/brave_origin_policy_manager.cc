/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_policy_manager.h"

#include "base/containers/map_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_origin/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_origin {

// static
BraveOriginPolicyManager* BraveOriginPolicyManager::GetInstance() {
  static base::NoDestructor<BraveOriginPolicyManager> instance;
  return instance.get();
}

void BraveOriginPolicyManager::Init(
    BraveOriginPolicyMap&& browser_policy_definitions,
    BraveOriginPolicyMap&& profile_policy_definitions,
    PrefService* local_state) {
  CHECK(!initialized_) << "BraveOriginPolicyManager already initialized";
  CHECK(local_state) << "BraveOriginPolicyManager local state should exist";

  browser_policy_definitions_ = std::move(browser_policy_definitions);
  profile_policy_definitions_ = std::move(profile_policy_definitions);
  local_state_ = local_state;
  initialized_ = true;

  // Notify observers that policies are now ready
  for (auto& observer : observers_) {
    observer.OnBraveOriginPoliciesReady();
  }
}

void BraveOriginPolicyManager::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);

  // If local state is already available, notify immediately
  if (local_state_) {
    observer->OnBraveOriginPoliciesReady();
  }
}

void BraveOriginPolicyManager::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

std::optional<bool> BraveOriginPolicyManager::GetPolicyValue(
    std::string_view pref_name,
    std::optional<std::string_view> profile_id) const {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  CHECK(local_state_) << "BraveOriginPolicyManager local state should exist";

  // First check browser-level policies
  const BraveOriginPolicyInfo* policy_info =
      base::FindOrNull(browser_policy_definitions_, pref_name);

  // If not found in browser policies, check profile policies
  if (!policy_info) {
    policy_info = base::FindOrNull(profile_policy_definitions_, pref_name);
  }

  if (!policy_info) {
    LOG(ERROR) << "Unknown pref name: " << pref_name;
    return std::nullopt;
  }

  // Get policies dict once and pass to internal helper
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(kBraveOriginPolicies);
  return GetPolicyValueInternal(*policy_info, policies_dict, profile_id);
}

PoliciesEnabledMap BraveOriginPolicyManager::GetAllBrowserPolicies() const {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  CHECK(local_state_) << "BraveOriginPolicyManager local state should exist";

  PoliciesEnabledMap policies;

  // Get policies dict once for all lookups
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(kBraveOriginPolicies);

  for (const auto& [pref_name, policy_info] : browser_policy_definitions_) {
    auto value = GetPolicyValueInternal(policy_info, policies_dict);
    policies[policy_info.policy_key] = value;
  }
  return policies;
}

PoliciesEnabledMap BraveOriginPolicyManager::GetAllProfilePolicies(
    std::string_view profile_id) const {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  CHECK(local_state_) << "BraveOriginPolicyManager local state should exist";

  PoliciesEnabledMap policies;

  // Get policies dict once for all lookups
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(kBraveOriginPolicies);

  for (const auto& [pref_name, policy_info] : profile_policy_definitions_) {
    auto value = GetPolicyValueInternal(policy_info, policies_dict, profile_id);
    policies[policy_info.policy_key] = value;
  }
  return policies;
}

void BraveOriginPolicyManager::SetBrowserPolicyValue(std::string_view pref_name,
                                                     bool value) {
  SetPolicyValueInternal(pref_name, value, browser_policy_definitions_,
                         /*profile_id=*/std::nullopt);
}

void BraveOriginPolicyManager::SetProfilePolicyValue(
    std::string_view pref_name,
    bool value,
    std::string_view profile_id) {
  CHECK(!profile_id.empty())
      << "Profile ID cannot be empty for profile policies";

  SetPolicyValueInternal(pref_name, value, profile_policy_definitions_,
                         profile_id);
}

void BraveOriginPolicyManager::SetPolicyValueInternal(
    std::string_view pref_name,
    bool value,
    const BraveOriginPolicyMap& policy_definitions,
    std::optional<std::string_view> profile_id) {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  CHECK(local_state_) << "BraveOriginPolicyManager local state should exist";

  // Check if this is a valid pref
  const BraveOriginPolicyInfo* policy_info =
      base::FindOrNull(policy_definitions, pref_name);
  if (!policy_info) {
    LOG(ERROR) << "Unknown " << (profile_id.has_value() ? "profile" : "browser")
               << " pref name: " << pref_name;
    return;
  }

  // Update the value in the dictionary
  ScopedDictPrefUpdate update(local_state_, kBraveOriginPolicies);
  std::string key =
      profile_id.has_value()
          ? GetBraveOriginProfilePrefKey(*policy_info, *profile_id)
          : GetBraveOriginBrowserPrefKey(*policy_info);
  update->Set(key, value);
}

// Helper function to get pref info from pref definitions
const BraveOriginPolicyInfo* BraveOriginPolicyManager::GetPrefInfo(
    std::string_view pref_name) {
  // First check browser-level policies
  const BraveOriginPolicyInfo* policy_info =
      base::FindOrNull(browser_policy_definitions_, pref_name);

  // If not found in browser policies, check profile policies
  if (!policy_info) {
    policy_info = base::FindOrNull(profile_policy_definitions_, pref_name);
  }

  return policy_info;
}

bool BraveOriginPolicyManager::IsInitialized() const {
  return initialized_;
}

void BraveOriginPolicyManager::Shutdown() {
  initialized_ = false;
  browser_policy_definitions_.clear();
  profile_policy_definitions_.clear();
  local_state_ = nullptr;
  observers_.Clear();
}

bool BraveOriginPolicyManager::GetPolicyValueInternal(
    const BraveOriginPolicyInfo& policy_info,
    const base::Value::Dict& policies_dict,
    std::optional<std::string_view> profile_id) const {
  std::string policy_key;
  if (!profile_id.has_value()) {
    // Browser-scoped preference
    policy_key = GetBraveOriginBrowserPrefKey(policy_info);
  } else {
    // Profile-scoped preference
    policy_key = GetBraveOriginProfilePrefKey(policy_info, profile_id.value());
  }

  const base::Value* policy_value = policies_dict.Find(policy_key);
  if (policy_value && policy_value->is_bool()) {
    return policy_value->GetBool();
  }

  // Return default value if no policy value found
  return policy_info.default_value;
}

BraveOriginPolicyManager::BraveOriginPolicyManager() = default;

BraveOriginPolicyManager::~BraveOriginPolicyManager() = default;

}  // namespace brave_origin
