/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_CONTEXT_H_
#define BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_CONTEXT_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/ref_counted.h"
#include "brave/browser/net/storage_partition_proxy_service.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/proxy_config_with_annotation.mojom.h"

namespace brave {

// Relays profile proxy updates and applies overrides for one NetworkContext.
class StoragePartitionProxyService::Context
    : public base::RefCounted<Context>,
      public network::mojom::ProxyConfigClient,
      public network::mojom::ProxyErrorClient {
 public:
  Context(StoragePartitionProxyService& owner,
          content::StoragePartitionConfig partition_config,
          network::mojom::NetworkContextParams* params);

  void Update();
  void FlushForTesting();

  // network::mojom::ProxyConfigClient:
  void OnProxyConfigUpdated(
      const net::ProxyConfigWithAnnotation& config) override;
  void FlushProxyConfig(FlushProxyConfigCallback callback) override;

  // network::mojom::ProxyErrorClient:
  void OnPACScriptError(int32_t line_number,
                        const std::string& details) override;
  void OnRequestMaybeFailedDueToProxySettings(int32_t net_error) override;

 private:
  friend class base::RefCounted<Context>;
  ~Context() override;

  bool UsesInheritedConfig() const;
  bool HasInheritedDynamicRouting() const;
  std::optional<net::ProxyConfigWithAnnotation> EffectiveConfig() const;

  const raw_ref<StoragePartitionProxyService> owner_;
  const content::StoragePartitionConfig partition_config_;
  std::optional<net::ProxyConfigWithAnnotation> inherited_config_;
  std::optional<net::ProxyConfigWithAnnotation> last_config_;
  mojo::Receiver<network::mojom::ProxyConfigClient> inherited_receiver_{this};
  mojo::Remote<network::mojom::ProxyConfigClient> client_;
  mojo::Remote<network::mojom::ProxyErrorClient> inherited_error_client_;
  mojo::Receiver<network::mojom::ProxyErrorClient> error_receiver_{this};
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_CONTEXT_H_
