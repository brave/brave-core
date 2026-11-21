// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/net/storage_partition_proxy_service.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/net/storage_partition_proxy_context.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace brave {

namespace {

constexpr const char* kProxyPrefs[] = {
    proxy_config::prefs::kProxy,
    proxy_config::prefs::kProxyOverrideRules,
};

}  // namespace

StoragePartitionProxyService::StoragePartitionProxyService(
    PrefService& prefs,
    PrefService& local_state,
    bool is_off_the_record)
    : prefs_(prefs),
      local_state_(local_state),
      is_off_the_record_(is_off_the_record) {
  const auto changed = base::BindRepeating(
      &StoragePartitionProxyService::OnSettingsChanged, base::Unretained(this));
  registrar_.Init(&prefs);
  local_state_registrar_.Init(&local_state);
  for (const char* name : kProxyPrefs) {
    if (prefs.FindPreference(name)) {
      registrar_.Add(name, changed);
    }
    if (local_state.FindPreference(name)) {
      local_state_registrar_.Add(name, changed);
    }
  }
}

StoragePartitionProxyService::~StoragePartitionProxyService() = default;

void StoragePartitionProxyService::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  weak_factory_.InvalidateWeakPtrs();
  contexts_.clear();
  overrides_.clear();
  registrar_.RemoveAll();
  local_state_registrar_.RemoveAll();
}

void StoragePartitionProxyService::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void StoragePartitionProxyService::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void StoragePartitionProxyService::SetProxyConfig(
    const content::StoragePartitionConfig& partition_config,
    const net::ProxyConfigWithAnnotation& config,
    Precedence precedence) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  overrides_.insert_or_assign(ValidatedKey(partition_config),
                              Override{config, precedence});
  OnSettingsChanged();
}

void StoragePartitionProxyService::ClearProxyConfig(
    const content::StoragePartitionConfig& partition_config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (overrides_.erase(ValidatedKey(partition_config))) {
    OnSettingsChanged();
  }
}

std::optional<net::ProxyConfigWithAnnotation>
StoragePartitionProxyService::GetProxyConfig(
    const content::StoragePartitionConfig& partition_config) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = overrides_.find(ValidatedKey(partition_config));
  return it == overrides_.end() ? std::nullopt
                                : std::make_optional(it->second.config);
}

void StoragePartitionProxyService::ConfigureNetworkContextParams(
    const content::StoragePartitionConfig& partition_config,
    network::mojom::NetworkContextParams* params) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  contexts_.push_back(base::MakeRefCounted<Context>(
      *this, ValidatedKey(partition_config), params));
}

content::StoragePartitionConfig StoragePartitionProxyService::ValidatedKey(
    const content::StoragePartitionConfig& partition_config) const {
  // StoragePartitionConfig::Create() forces in-memory storage for
  // off-the-record contexts, so an on-disk config belongs to another profile.
  CHECK(!is_off_the_record_ || partition_config.in_memory());
  content::StoragePartitionConfig result = partition_config;
  result.set_fallback_to_partition_domain_for_blob_urls(
      content::StoragePartitionConfig::FallbackMode::kNone);
  return result;
}

StoragePartitionProxyService::SettingsControl
StoragePartitionProxyService::GetSettingsControl() const {
  SettingsControl control = SettingsControl::kNone;
  for (const PrefService* prefs : {&*prefs_, &*local_state_}) {
    for (const char* name : kProxyPrefs) {
      const auto* pref = prefs->FindPreference(name);
      if (!pref) {
        continue;
      }
      if (pref->IsManaged() || pref->IsManagedByCustodian()) {
        return SettingsControl::kPolicy;
      }
      // Only the extension and command-line stores rank between policy and
      // the user store.
      if (!pref->IsUserModifiable()) {
        control = SettingsControl::kExtensionOrCommandLine;
      }
    }
  }
  return control;
}

void StoragePartitionProxyService::OnSettingsChanged() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& context : contexts_) {
    context->Update();
  }
}

void StoragePartitionProxyService::FlushForTesting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Flushing can dispatch disconnects and remove contexts from the service.
  const auto contexts = contexts_;
  for (const auto& context : contexts) {
    context->FlushForTesting();
  }
}

void StoragePartitionProxyService::RemoveContext(Context* context) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::erase_if(contexts_, [context](const auto& entry) {
    return entry.get() == context;
  });
}

}  // namespace brave
