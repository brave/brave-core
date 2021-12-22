/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/brave_federated_learning_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "brave/components/brave_federated_learning/brave_federated_data_service.h"
#include "brave/components/brave_federated_learning/brave_federated_learning_features.h"
#include "brave/components/brave_federated_learning/brave_operational_patterns.h"
#include "brave/components/brave_federated_learning/brave_operational_patterns_features.h"
#include "brave/components/brave_federated_learning/data_stores/data_store.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave {

namespace federated_learning {

BraveFederatedLearningService::BraveFederatedLearningService(
    PrefService* prefs,
    PrefService* local_state,
    const base::FilePath brave_federated_learning_path,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs),
      local_state_(local_state),
      brave_federated_learning_path_(brave_federated_learning_path),
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
      base::BindRepeating(&BraveFederatedLearningService::OnPreferenceChanged,
                          base::Unretained(this)));
}

void BraveFederatedLearningService::Start() {
  if (IsFederatedLearningEnabled()) {
    base::FilePath db_path(
        brave_federated_learning_path_.AppendASCII("data_store.sqlite"));

    data_service_.reset(new DataStoreService(db_path));
    data_service_->Init();
  }

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

bool BraveFederatedLearningService::IsFederatedLearningEnabled() {
  return brave::federated_learning::features::IsFederatedLearningEnabled();
}

bool BraveFederatedLearningService::IsOperationalPatternsEnabled() {
  return operational_patterns::features::IsOperationalPatternsEnabled();
}

bool BraveFederatedLearningService::IsP3AEnabled() {
  return local_state_->GetBoolean(brave::kP3AEnabled);
}

}  // namespace federated_learning

}  // namespace brave
