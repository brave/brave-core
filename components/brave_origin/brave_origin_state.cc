/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_state.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_origin/pref_names.h"

namespace brave_origin {

BraveOriginState::BraveOriginState()
    : is_brave_origin_user_(false),
      initialized_(false),
      was_managed_before_brave_origin_(false) {}

BraveOriginState::~BraveOriginState() = default;

BraveOriginState* BraveOriginState::GetInstance() {
  static base::NoDestructor<BraveOriginState> instance;
  return instance.get();
}

void BraveOriginState::Initialize() {
  // TODO(https://github.com/brave/brave-browser/issues/47463)
  // Get the actual purchase state from SKU service.
  is_brave_origin_user_ = base::FeatureList::IsEnabled(features::kBraveOrigin);
  initialized_ = true;
}

bool BraveOriginState::IsBraveOriginUser() const {
  if (!initialized_) {
    return false;
  }
  return is_brave_origin_user_;
}

void BraveOriginState::AddBraveOriginControlledPref(
    const std::string& pref_name) {
  brave_origin_controlled_prefs_.insert(pref_name);
}

bool BraveOriginState::IsPrefControlledByBraveOrigin(
    const std::string& pref_name) const {
  return brave_origin_controlled_prefs_.contains(pref_name);
}

void BraveOriginState::ClearBraveOriginControlledPrefs() {
  brave_origin_controlled_prefs_.clear();
}

void BraveOriginState::SetWasManagedBeforeBraveOrigin(bool was_managed) {
  was_managed_before_brave_origin_ = was_managed;
}

bool BraveOriginState::WasManagedBeforeBraveOrigin() const {
  return was_managed_before_brave_origin_;
}

}  // namespace brave_origin
