/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_federated_learning/brave_federated_learning_service.h"

#include "brave/components/brave_federated_learning/brave_operational_patterns.h"
#include "brave/components/brave_federated_learning/brave_operational_patterns_features.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave {

BraveFederatedLearningService::BraveFederatedLearningService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : local_state_(pref_service), url_loader_factory_(url_loader_factory) {}

BraveFederatedLearningService::~BraveFederatedLearningService() {}

void BraveFederatedLearningService::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  BraveOperationalPatterns::RegisterLocalStatePrefs(registry);
}

void BraveFederatedLearningService::Start() {
  operational_patterns_.reset(
      new BraveOperationalPatterns(local_state_, url_loader_factory_));

  InitPrefChangeRegistrar();

  if (IsP3AEnabled() && IsOperationalPatternsEnabled()) {
    operational_patterns_->Start();
  }
}

void BraveFederatedLearningService::InitPrefChangeRegistrar() {
  local_state_change_registrar_.Init(local_state_);
  local_state_change_registrar_.Add(
      brave::kP3AEnabled,
      base::BindRepeating(&BraveFederatedLearningService::OnPreferenceChanged,
                          base::Unretained(this)));
}

void BraveFederatedLearningService::OnPreferenceChanged(
    const std::string& key) {
  if (!IsP3AEnabled() || !IsOperationalPatternsEnabled()) {
    operational_patterns_->Stop();
  } else if (IsP3AEnabled() && IsOperationalPatternsEnabled()) {
    operational_patterns_->Start();
  }
}

bool BraveFederatedLearningService::IsOperationalPatternsEnabled() {
  return operational_patterns::features::IsOperationalPatternsEnabled();
}

bool BraveFederatedLearningService::IsP3AEnabled() {
  return local_state_->GetBoolean(brave::kP3AEnabled);
}

}  // namespace brave
