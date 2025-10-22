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
  observers_.Notify(&brave_policy::BravePolicyObserver::OnBravePoliciesReady);
}

void BraveOriginPolicyManager::AddObserver(
    brave_policy::BravePolicyObserver* observer) {
  observers_.AddObserver(observer);

  // If local state is already available, notify immediately
  if (local_state_) {
    observer->OnBravePoliciesReady();
  }
}

void BraveOriginPolicyManager::RemoveObserver(
    brave_policy::BravePolicyObserver* observer) {
  observers_.RemoveObserver(observer);
}

std::optional<bool> BraveOriginPolicyManager::GetPolicyValue(
    std::string_view policy_key,
    std::optional<std::string_view> profile_id) const {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  CHECK(local_state_) << "BraveOriginPolicyManager local state should exist";

  const BraveOriginPolicyInfo* policy_info = GetPolicyInfo(policy_key);
  if (!policy_info) {
    LOG(ERROR) << "Unknown policy key: " << policy_key;
    return std::nullopt;
  }

  // Get policies dict once and pass to internal helper
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(kBraveOriginPolicies);
  return GetPolicyValueInternal(policy_key, policy_info->default_value,
                                policies_dict, profile_id);
}

bool BraveOriginPolicyManager::IsBrowserPolicy(
    std::string_view policy_key) const {
  return browser_policy_definitions_.contains(policy_key);
}

bool BraveOriginPolicyManager::IsProfilePolicy(
    std::string_view policy_key) const {
  return profile_policy_definitions_.contains(policy_key);
}

PoliciesEnabledMap BraveOriginPolicyManager::GetAllBrowserPolicies() const {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  CHECK(local_state_) << "BraveOriginPolicyManager local state should exist";

  PoliciesEnabledMap policies;

  // Get policies dict once for all lookups
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(kBraveOriginPolicies);

  for (const auto& [policy_key, policy_info] : browser_policy_definitions_) {
    bool value = GetPolicyValueInternal(policy_key, policy_info.default_value,
                                        policies_dict, std::nullopt);
    policies[policy_key] = value;
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

  for (const auto& [policy_key, policy_info] : profile_policy_definitions_) {
    bool value = GetPolicyValueInternal(policy_key, policy_info.default_value,
                                        policies_dict, profile_id);
    policies[policy_key] = value;
  }
  return policies;
}

void BraveOriginPolicyManager::SetPolicyValue(
    std::string_view policy_key,
    bool value,
    std::optional<std::string_view> profile_id) {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  CHECK(local_state_) << "BraveOriginPolicyManager local state should exist";

  // Validate that this is a known policy key
  if (!GetPolicyInfo(policy_key)) {
    LOG(ERROR) << "Unknown " << (profile_id.has_value() ? "profile" : "browser")
               << " policy key: " << policy_key;
    return;
  }

  // Update the value in the dictionary
  ScopedDictPrefUpdate update(local_state_, kBraveOriginPolicies);
  std::string key = GetBraveOriginPrefKey(policy_key, profile_id);
  update->Set(key, value);

  // Notify observers of the policy change
  if (profile_id.has_value()) {
    observers_.Notify(
        &brave_policy::BravePolicyObserver::OnProfilePolicyChanged, policy_key,
        profile_id.value());
  } else {
    observers_.Notify(
        &brave_policy::BravePolicyObserver::OnBrowserPolicyChanged, policy_key);
  }
}

// Helper function to get policy info from policy definitions
const BraveOriginPolicyInfo* BraveOriginPolicyManager::GetPolicyInfo(
    std::string_view policy_key) const {
  // First check browser-level policies
  const BraveOriginPolicyInfo* policy_info =
      base::FindOrNull(browser_policy_definitions_, policy_key);

  // If not found in browser policies, check profile policies
  if (!policy_info) {
    policy_info = base::FindOrNull(profile_policy_definitions_, policy_key);
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
    std::string_view policy_key,
    bool default_value,
    const base::Value::Dict& policies_dict,
    std::optional<std::string_view> profile_id) const {
  std::string pref_key = GetBraveOriginPrefKey(policy_key, profile_id);
  const base::Value* policy_value = policies_dict.Find(pref_key);
  if (policy_value && policy_value->is_bool()) {
    return policy_value->GetBool();
  }

  // Return default value if no policy value found
  return default_value;
}

BraveOriginPolicyManager::BraveOriginPolicyManager() = default;

BraveOriginPolicyManager::~BraveOriginPolicyManager() = default;

}  // namespace brave_origin
