/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_BROWSER_EPHEMERAL_STORAGE_PARTITION_H_
#define BRAVE_CONTENT_BROWSER_EPHEMERAL_STORAGE_PARTITION_H_

#include <string>
#include <utility>

#include "content/public/browser/storage_partition.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "base/memory/ref_counted.h"

namespace content {

using EphemeralStorageParitionMapKey =
    std::pair<content::BrowserContext*, std::string>;

class StoragePartitionImpl;

// EphemeralStoragePartition manages an in-memory NetworkingContext to provide
// ephemeral cookie storage.
class EphemeralStoragePartition
    : public base::RefCountedThreadSafe<EphemeralStoragePartition> {
 public:
  explicit EphemeralStoragePartition(EphemeralStorageParitionMapKey key);
  EphemeralStoragePartition(const EphemeralStoragePartition&) = delete;
  EphemeralStoragePartition& operator=(const EphemeralStoragePartition&) =
      delete;

  static EphemeralStoragePartition* Get(BrowserContext*,
                                        std::string storage_domain);
  static scoped_refptr<EphemeralStoragePartition> GetOrCreate(
      BrowserContext* browser_context,
      std::string storage_domain);

  void CreateRestrictedCookieManagerForScript(
      const url::Origin& origin,
      const net::SiteForCookies& site_for_cookies,
      const url::Origin& top_frame_origin,
      int process_id,
      int routing_id,
      mojo::PendingReceiver<network::mojom::RestrictedCookieManager> receiver,
      mojo::PendingRemote<network::mojom::CookieAccessObserver>
          cookie_observer);

 private:
  virtual ~EphemeralStoragePartition();
  friend class base::RefCountedThreadSafe<EphemeralStoragePartition>;

  void InitNetworkContext();
  network::mojom::NetworkContext* GetNetworkContext();

  EphemeralStorageParitionMapKey key_;
  BrowserContext* browser_context_;

  mojo::Remote<network::mojom::NetworkContext> network_context_;
  mojo::Remote<network::mojom::CookieManager>
      cookie_manager_for_browser_process_;

  base::WeakPtrFactory<EphemeralStoragePartition> weak_factory_{this};
};

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_EPHEMERAL_STORAGE_PARTITION_H_
