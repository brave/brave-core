/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/brave_federated_learning_service.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_federated_learning/brave_operational_patterns.h"
#include "brave/components/brave_federated_learning/brave_operational_patterns_features.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave {

BraveFederatedLearningService::BraveFederatedLearningService(
    PrefService* prefs,
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs),
      local_state_(local_state),
      url_loader_factory_(url_loader_factory) {
  InitPrefChangeRegistrar();
  Start();
}

BraveFederatedLearningService::~BraveFederatedLearningService() {}

void BraveFederatedLearningService::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  BraveOperationalPatterns::RegisterPrefs(registry);
}

void BraveFederatedLearningService::InitPrefChangeRegistrar() {
  local_state_change_registrar_.Init(local_state_);
  local_state_change_registrar_.Add(
      brave::kP3AEnabled,
      base::BindRepeating(
          &brave::BraveFederatedLearningService::OnPreferenceChanged,
          base::Unretained(this)));
}

void BraveFederatedLearningService::Start() {
  if (ShouldStartOperationalPatterns()) {
    operational_patterns_.reset(
        new BraveOperationalPatterns(prefs_, url_loader_factory_));
    operational_patterns_->Start();
  }
}

bool BraveFederatedLearningService::ShouldStartOperationalPatterns() {
  return IsP3AEnabled() && IsOperationalPatternsEnabled();
}

void BraveFederatedLearningService::OnPreferenceChanged(
    const std::string& key) {
  if (operational_patterns_) {
    if (ShouldStartOperationalPatterns()) {
      operational_patterns_->Start();
    } else {
      operational_patterns_->Stop();
    }
  } else {
    Start();
  }
}

bool BraveFederatedLearningService::IsOperationalPatternsEnabled() {
  return operational_patterns::features::IsOperationalPatternsEnabled();
}

bool BraveFederatedLearningService::IsP3AEnabled() {
  return local_state_->GetBoolean(brave::kP3AEnabled);
}

}  // namespace brave
