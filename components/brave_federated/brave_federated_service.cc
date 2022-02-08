/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/brave_federated_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/path_service.h"
#include "brave/components/brave_federated/data_store_service.h"
#include "brave/components/brave_federated/data_stores/data_store.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/operational_patterns.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_federated {

BraveFederatedService::BraveFederatedService(
    PrefService* prefs,
    PrefService* local_state,
    const base::FilePath& browser_context_path,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs),
      local_state_(local_state),
      browser_context_path_(browser_context_path),
      url_loader_factory_(url_loader_factory) {
  InitPrefChangeRegistrar();
  Start();
}

BraveFederatedService::~BraveFederatedService() {}

void BraveFederatedService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  OperationalPatterns::RegisterPrefs(registry);
}

void BraveFederatedService::InitPrefChangeRegistrar() {
  local_state_change_registrar_.Init(local_state_);
  local_state_change_registrar_.Add(
      brave::kP3AEnabled,
      base::BindRepeating(&BraveFederatedService::OnPreferenceChanged,
                          base::Unretained(this)));
}

void BraveFederatedService::Start() {
  base::FilePath db_path(
      browser_context_path_.AppendASCII("data_store.sqlite"));

  data_store_service_.reset(new DataStoreService(db_path));
  data_store_service_->Init();

  if (ShouldStartOperationalPatterns()) {
    operational_patterns_.reset(
        new OperationalPatterns(prefs_, url_loader_factory_));
    operational_patterns_->Start();
  }
}

bool BraveFederatedService::ShouldStartOperationalPatterns() {
  return IsP3AEnabled() && IsOperationalPatternsEnabled();
}

void BraveFederatedService::OnPreferenceChanged(const std::string& key) {
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

bool BraveFederatedService::IsFederatedLearningEnabled() {
  return brave_federated::features::IsFederatedLearningEnabled();
}

bool BraveFederatedService::IsOperationalPatternsEnabled() {
  return brave_federated::features::IsOperationalPatternsEnabled();
}

bool BraveFederatedService::IsP3AEnabled() {
  return local_state_->GetBoolean(brave::kP3AEnabled);
}

}  // namespace brave_federated
