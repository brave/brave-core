/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/ephemeral_storage_partition.h"

#include <map>
#include "base/no_destructor.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_client.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace content {

namespace {

using EphemeralStoragePartitionMap =
    std::map<EphemeralStorageParitionMapKey, EphemeralStoragePartition*>;

// This map allows EphemeralStorageTabHelper to manage the lifetime of ephemeral
// storage partitions. When the last reference to an EphemeralStoragePartition
// is cleared, the partition will be removed from the map and its storage
// deleted.
EphemeralStoragePartitionMap& ephemeral_storage_partition_map() {
  static base::NoDestructor<EphemeralStoragePartitionMap>
      ephemeral_storage_partition_map;
  return *ephemeral_storage_partition_map.get();
}

}  // namespace

EphemeralStoragePartition* EphemeralStoragePartition::Get(
    BrowserContext* browser_context,
    std::string storage_domain) {
  EphemeralStorageParitionMapKey key =
      std::make_pair(browser_context, storage_domain);
  auto it = ephemeral_storage_partition_map().find(key);
  return it != ephemeral_storage_partition_map().end() ? it->second : nullptr;
}

scoped_refptr<EphemeralStoragePartition> EphemeralStoragePartition::GetOrCreate(
    BrowserContext* browser_context,
    std::string storage_domain) {
  if (EphemeralStoragePartition* existing =
          Get(browser_context, storage_domain)) {
    return existing;
  }

  EphemeralStorageParitionMapKey key =
      std::make_pair(browser_context, storage_domain);
  return base::MakeRefCounted<EphemeralStoragePartition>(key);
}

EphemeralStoragePartition::EphemeralStoragePartition(
    EphemeralStorageParitionMapKey key)
    : key_(key) {
  ephemeral_storage_partition_map().emplace(key, this);
}

EphemeralStoragePartition::~EphemeralStoragePartition() {
  ephemeral_storage_partition_map().erase(key_);
}

network::mojom::NetworkContext* EphemeralStoragePartition::GetNetworkContext() {
  if (!network_context_.is_bound())
    InitNetworkContext();
  return network_context_.get();
}

void EphemeralStoragePartition::InitNetworkContext() {
  BrowserContext* browser_context = key_.first;
  network::mojom::NetworkContextParamsPtr context_params =
      network::mojom::NetworkContextParams::New();
  network::mojom::CertVerifierCreationParamsPtr cert_verifier_creation_params =
      network::mojom::CertVerifierCreationParams::New();
  auto relative_partition_path =
      base::FilePath(std::string("ephemeral") + key_.second);
  GetContentClient()->browser()->ConfigureNetworkContextParams(
      browser_context, /* in_memory = */ true, relative_partition_path,
      context_params.get(), cert_verifier_creation_params.get());

  network_context_.reset();
  GetNetworkService()->CreateNetworkContext(
      network_context_.BindNewPipeAndPassReceiver(), std::move(context_params));
  DCHECK(network_context_);

  network_context_.set_disconnect_handler(
      base::BindOnce(&EphemeralStoragePartition::InitNetworkContext,
                     weak_factory_.GetWeakPtr()));
}

void EphemeralStoragePartition::CreateRestrictedCookieManagerForScript(
    const url::Origin& origin,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    int process_id,
    int routing_id,
    mojo::PendingReceiver<network::mojom::RestrictedCookieManager> receiver,
    mojo::PendingRemote<network::mojom::CookieAccessObserver> cookie_observer) {
  network::mojom::RestrictedCookieManagerRole role =
      network::mojom::RestrictedCookieManagerRole::SCRIPT;
  BrowserContext* browser_context = key_.first;
  if (!GetContentClient()->browser()->WillCreateRestrictedCookieManager(
          role, browser_context, origin, site_for_cookies, top_frame_origin,
          /* is_service_worker = */ false, process_id, routing_id, &receiver)) {
    GetNetworkContext()->GetRestrictedCookieManager(
        std::move(receiver), role, origin, site_for_cookies, top_frame_origin,
        std::move(cookie_observer));
  }
}

}  // namespace content
