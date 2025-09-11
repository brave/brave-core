/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_policy_manager.h"

#include "base/logging.h"
#include "base/no_destructor.h"
#include "brave/components/brave_origin/brave_origin_constants.h"
#include "components/prefs/pref_service.h"

namespace brave_origin {

// static
BraveOriginPolicyManager* BraveOriginPolicyManager::GetInstance() {
  static base::NoDestructor<BraveOriginPolicyManager> instance;
  return instance.get();
}

void BraveOriginPolicyManager::Init(BraveOriginPolicyMap policy_definitions) {
  CHECK(!initialized_) << "BraveOriginPolicyManager already initialized";

  policy_definitions_ = std::move(policy_definitions);
  initialized_ = true;
}

void BraveOriginPolicyManager::SetLocalState(PrefService* local_state) {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  local_state_ = local_state;

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

const base::Value& BraveOriginPolicyManager::GetPolicyValue(
    const std::string& pref_name,
    const std::string& profile_id) const {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";

  auto it = policy_definitions_.find(pref_name);
  if (it == policy_definitions_.end()) {
    LOG(ERROR) << "Unknown pref name: " << pref_name;
    static const base::Value empty_value;
    return empty_value;
  }

  const BraveOriginPolicyInfo& policy_info = it->second;

  // If local state is available, try to get the policy value
  if (local_state_) {
    const base::Value::Dict& policies_dict =
        local_state_->GetDict(kBraveOriginPolicies);

    std::string policy_key = GetPolicyKey(policy_info, profile_id);
    const base::Value* policy_value = policies_dict.Find(policy_key);
    if (policy_value) {
      return *policy_value;
    }
  }

  // Return default value if no policy value found
  return policy_info.default_value;
}

const BraveOriginPolicyMap& BraveOriginPolicyManager::GetDefinitions() const {
  CHECK(initialized_) << "BraveOriginPolicyManager not initialized";
  return policy_definitions_;
}

// Helper function to get pref info from pref definitions
const BraveOriginPolicyInfo* BraveOriginPolicyManager::GetPrefInfo(
    const std::string& pref_name) {
  auto it = policy_definitions_.find(pref_name);
  if (it != policy_definitions_.end()) {
    return &it->second;
  }
  return nullptr;
}

// static
std::string BraveOriginPolicyManager::GetPolicyKey(
    const BraveOriginPolicyInfo& policy_info,
    const std::string& profile_id) {
  // For profile-scoped prefs, profile_id is required
  if (policy_info.scope == BraveOriginPolicyScope::kProfile) {
    CHECK(!profile_id.empty())
        << "Profile ID required for profile-scoped pref: "
        << policy_info.pref_name;
    return profile_id + "." + policy_info.pref_name;
  }

  // For global prefs, use pref_name directly
  return policy_info.pref_name;
}

bool BraveOriginPolicyManager::IsInitialized() const {
  return initialized_;
}

BraveOriginPolicyManager::BraveOriginPolicyManager() : initialized_(false) {}

BraveOriginPolicyManager::~BraveOriginPolicyManager() = default;

}  // namespace brave_origin
