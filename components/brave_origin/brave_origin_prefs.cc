/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_prefs.h"

#include "base/logging.h"
#include "base/no_destructor.h"

namespace brave_origin {

// static
BraveOriginPrefs* BraveOriginPrefs::GetInstance() {
  static base::NoDestructor<BraveOriginPrefs> instance;
  return instance.get();
}

void BraveOriginPrefs::Init(
    BraveOriginPrefMap pref_definitions,
    base::flat_map<std::string, std::string> policy_mappings) {
  if (initialized_) {
    LOG(WARNING) << "BraveOriginPrefs already initialized";
    return;
  }

  pref_definitions_ = std::move(pref_definitions);
  policy_mappings_ = std::move(policy_mappings);
  initialized_ = true;

  VLOG(1) << "BraveOriginPrefs initialized with " << pref_definitions_.size()
          << " pref definitions";
}

const BraveOriginPrefMap& BraveOriginPrefs::GetPrefDefinitions() const {
  DCHECK(initialized_) << "BraveOriginPrefs not initialized";
  return pref_definitions_;
}

const base::flat_map<std::string, std::string>&
BraveOriginPrefs::GetPolicyMappings() const {
  DCHECK(initialized_) << "BraveOriginPrefs not initialized";
  return policy_mappings_;
}

bool BraveOriginPrefs::IsInitialized() const {
  return initialized_;
}

BraveOriginPrefs::BraveOriginPrefs() : initialized_(false) {}

BraveOriginPrefs::~BraveOriginPrefs() = default;

}  // namespace brave_origin
