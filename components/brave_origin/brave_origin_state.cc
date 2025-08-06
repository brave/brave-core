/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_state.h"

#include "base/logging.h"
#include "base/no_destructor.h"
#include "brave/components/brave_origin/features.h"

BraveOriginState::BraveOriginState()
    : is_brave_origin_user_(false), initialized_(false) {}

BraveOriginState::~BraveOriginState() = default;

BraveOriginState* BraveOriginState::GetInstance() {
  static base::NoDestructor<BraveOriginState> instance;
  return instance.get();
}

void BraveOriginState::Initialize() {
  // TODO(https://github.com/brave/brave-browser/issues/47463)
  // Get the actual purchase state from SKU service.
  is_brave_origin_user_ =
      base::FeatureList::IsEnabled(brave_origin::features::kBraveOrigin);
  initialized_ = true;
}

bool BraveOriginState::IsBraveOriginUser() const {
  if (!initialized_) {
    return false;
  }
  return is_brave_origin_user_;
}

void BraveOriginState::AddObserver(BraveOriginStateObserver* observer) {
  observers_.AddObserver(observer);
}

void BraveOriginState::RemoveObserver(BraveOriginStateObserver* observer) {
  observers_.RemoveObserver(observer);
}