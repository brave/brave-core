/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/storage_partition_proxy_context.h"

#include <utility>

#include "base/functional/bind.h"
#include "net/proxy_resolution/proxy_config.h"

namespace brave {

StoragePartitionProxyService::Context::Context(
    StoragePartitionProxyService& owner,
    content::StoragePartitionConfig partition_config,
    network::mojom::NetworkContextParams* params)
    : owner_(owner),
      partition_config_(std::move(partition_config)),
      inherited_config_(std::move(params->initial_proxy_config)) {
  if (params->proxy_config_client_receiver) {
    inherited_receiver_.Bind(std::move(params->proxy_config_client_receiver));
  }
  if (params->proxy_error_client) {
    inherited_error_client_.Bind(std::move(params->proxy_error_client));
  }
  params->proxy_error_client = error_receiver_.BindNewPipeAndPassRemote();
  params->proxy_config_client_receiver = client_.BindNewPipeAndPassReceiver();
  last_config_ = EffectiveConfig();
  params->initial_proxy_config = last_config_;
  client_.set_disconnect_handler(
      base::BindOnce(&StoragePartitionProxyService::RemoveContext,
                     owner.weak_factory_.GetWeakPtr(), base::Unretained(this)));
  // Keep Chrome's polling remote so inherited settings stay current even
  // while a local override is active.
}

StoragePartitionProxyService::Context::~Context() = default;

void StoragePartitionProxyService::Context::Update() {
  auto config = EffectiveConfig();
  if (!config) {
    return;
  }
  if (!last_config_ || !last_config_->value().Equals(config->value()) ||
      last_config_->traffic_annotation() != config->traffic_annotation()) {
    last_config_ = config;
    client_->OnProxyConfigUpdated(*config);
  }
}

void StoragePartitionProxyService::Context::FlushForTesting() {
  if (inherited_receiver_.is_bound()) {
    inherited_receiver_.FlushForTesting();
  }
  client_.FlushForTesting();
}

void StoragePartitionProxyService::Context::OnProxyConfigUpdated(
    const net::ProxyConfigWithAnnotation& config) {
  inherited_config_ = config;
  Update();
}

void StoragePartitionProxyService::Context::FlushProxyConfig(
    FlushProxyConfigCallback callback) {
  client_->FlushProxyConfig(std::move(callback));
}

void StoragePartitionProxyService::Context::OnPACScriptError(
    int32_t line_number,
    const std::string& details) {
  if (!UsesInheritedConfig()) {
    for (auto& observer : owner_->observers_) {
      observer.OnPACScriptError(partition_config_, line_number, details);
    }
  } else if (inherited_error_client_.is_bound()) {
    inherited_error_client_->OnPACScriptError(line_number, details);
  }
}

void StoragePartitionProxyService::Context::
    OnRequestMaybeFailedDueToProxySettings(int32_t net_error) {
  if (!UsesInheritedConfig()) {
    for (auto& observer : owner_->observers_) {
      observer.OnRequestMaybeFailedDueToProxySettings(partition_config_,
                                                      net_error);
    }
  } else if (inherited_error_client_.is_bound()) {
    inherited_error_client_->OnRequestMaybeFailedDueToProxySettings(net_error);
  }
}

bool StoragePartitionProxyService::Context::UsesInheritedConfig() const {
  auto it = owner_->overrides_.find(partition_config_);
  if (it == owner_->overrides_.end()) {
    return true;
  }
  switch (owner_->GetSettingsControl()) {
    case SettingsControl::kPolicy:
      return true;
    case SettingsControl::kExtensionOrCommandLine:
      if (it->second.precedence ==
          Precedence::kYieldToExtensionAndCommandLine) {
        return true;
      }
      // Override rules from these sources are superseded by the override.
      return HasInheritedDynamicRouting();
    case SettingsControl::kNone:
      // Override rules without a known source are kept as a precaution.
      return HasInheritedDynamicRouting() ||
             (inherited_config_ &&
              !inherited_config_->value().proxy_override_rules().empty());
  }
}

bool StoragePartitionProxyService::Context::HasInheritedDynamicRouting() const {
  // Dynamic routing rules are only provisioned by enterprise policy.
  if (!inherited_config_) {
    return false;
  }
  const auto& routing = inherited_config_->value().dynamic_routing_config();
  return !routing.routing_rules.empty() || routing.is_update_in_progress;
}

std::optional<net::ProxyConfigWithAnnotation>
StoragePartitionProxyService::Context::EffectiveConfig() const {
  if (!UsesInheritedConfig()) {
    return owner_->overrides_.at(partition_config_).config;
  }
  if (inherited_config_ || !last_config_) {
    return inherited_config_;
  }
  // The inherited configuration is still pending, but the network context has
  // already received an override that must no longer apply. It cannot return
  // to the pending state, so block requests until the inherited configuration
  // arrives rather than use the stale override or connect directly.
  net::ProxyConfig blocked;
  blocked.proxy_rules().type = net::ProxyConfig::ProxyRules::Type::PROXY_LIST;
  blocked.proxy_rules().bypass_rules.AddRulesToSubtractImplicit();
  return net::ProxyConfigWithAnnotation(blocked,
                                        last_config_->traffic_annotation());
}

}  // namespace brave
