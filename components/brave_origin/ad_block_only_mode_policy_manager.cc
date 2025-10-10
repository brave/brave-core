/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/ad_block_only_mode_policy_manager.h"

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_origin {

// static
AdBlockOnlyModePolicyManager* AdBlockOnlyModePolicyManager::GetInstance() {
  static base::NoDestructor<AdBlockOnlyModePolicyManager> instance;
  return instance.get();
}

void AdBlockOnlyModePolicyManager::Init(PrefService* local_state) {
  CHECK(!initialized_) << "AdBlockOnlyModePolicyManager already initialized";
  CHECK(local_state) << "AdBlockOnlyModePolicyManager local state should exist";

  local_state_ = local_state;
  initialized_ = true;

  if (base::FeatureList::IsEnabled(brave_shields::features::kAdblockOnlyMode)) {
    pref_change_registrar_.Init(local_state_);
    pref_change_registrar_.Add(
        brave_shields::prefs::kAdBlockOnlyModeEnabled,
        base::BindRepeating(
            &AdBlockOnlyModePolicyManager::OnAdBlockOnlyModeChanged,
            base::Unretained(this)));

    OnAdBlockOnlyModeChanged();
  }
}

void AdBlockOnlyModePolicyManager::Shutdown() {
  pref_change_registrar_.RemoveAll();
  observers_.Clear();
  local_state_ = nullptr;
  initialized_ = false;
}

bool AdBlockOnlyModePolicyManager::IsInitialized() const {
  return initialized_;
}

void AdBlockOnlyModePolicyManager::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);

  if (local_state_ &&
      base::FeatureList::IsEnabled(brave_shields::features::kAdblockOnlyMode)) {
    // Notify the observer about Ad Block Only mode if local state is already
    // set.
    observer->OnAdBlockOnlyModePoliciesChanged();
  }
}

void AdBlockOnlyModePolicyManager::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool AdBlockOnlyModePolicyManager::IsAdBlockOnlyModeEnabled() const {
  if (!local_state_) {
    return false;
  }

  return base::FeatureList::IsEnabled(
             brave_shields::features::kAdblockOnlyMode) &&
         local_state_->GetBoolean(
             brave_shields::prefs::kAdBlockOnlyModeEnabled);
}

void AdBlockOnlyModePolicyManager::OnAdBlockOnlyModeChanged() {
  observers_.Notify(&Observer::OnAdBlockOnlyModePoliciesChanged);
}

AdBlockOnlyModePolicyManager::AdBlockOnlyModePolicyManager() = default;

AdBlockOnlyModePolicyManager::~AdBlockOnlyModePolicyManager() = default;

}  // namespace brave_origin
