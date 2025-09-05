/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_pref_definitions.h"

#include "base/logging.h"
#include "base/no_destructor.h"

namespace brave_origin {

// static
BraveOriginPrefDefinitions* BraveOriginPrefDefinitions::GetInstance() {
  static base::NoDestructor<BraveOriginPrefDefinitions> instance;
  return instance.get();
}

void BraveOriginPrefDefinitions::Init(BraveOriginPrefMap pref_definitions) {
  if (initialized_) {
    LOG(WARNING) << "BraveOriginPrefDefinitions already initialized";
    return;
  }

  pref_definitions_ = std::move(pref_definitions);
  initialized_ = true;

  VLOG(1) << "BraveOriginPrefDefinitions initialized with "
          << pref_definitions_.size() << " pref definitions";
}

const BraveOriginPrefMap& BraveOriginPrefDefinitions::GetAll() const {
  DCHECK(initialized_) << "BraveOriginPrefDefinitions not initialized";
  return pref_definitions_;
}

// Helper function to get pref info from pref definitions
const BraveOriginPrefInfo* BraveOriginPrefDefinitions::GetPrefInfo(
    const std::string& pref_name) {
  auto it = pref_definitions_.find(pref_name);
  if (it != pref_definitions_.end()) {
    return &it->second;
  }
  return nullptr;
}

bool BraveOriginPrefDefinitions::IsInitialized() const {
  return initialized_;
}

BraveOriginPrefDefinitions::BraveOriginPrefDefinitions()
    : initialized_(false) {}

BraveOriginPrefDefinitions::~BraveOriginPrefDefinitions() = default;

}  // namespace brave_origin
