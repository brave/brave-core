/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_TLD_EPHEMERAL_LIFETIME_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_TLD_EPHEMERAL_LIFETIME_H_

#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "content/public/browser/storage_partition_config.h"
#include "url/origin.h"

namespace content {

class BrowserContext;

}  // namespace content

namespace ephemeral_storage {

// TLD storage is keyed by the BrowserContext (profile) and the TLD-specific
// security domain.
using TLDEphemeralLifetimeKey =
    std::pair<content::BrowserContext*, std::string>;

// This class is responsible for managing the lifetime of ephemeral storage
// cookies. Each instance is shared by each top-level frame with the same
// TLDEphemeralLifetimeKey. When the last top-level frame holding a reference
// is destroyed or navigates to a new storage domain, storage will be
// cleared.
class TLDEphemeralLifetime : public base::RefCounted<TLDEphemeralLifetime> {
 public:
  using OnDestroyCallback = base::OnceCallback<void(const std::string&)>;

  TLDEphemeralLifetime(
      const TLDEphemeralLifetimeKey& key,
      const content::StoragePartitionConfig& storage_partition_config);
  static TLDEphemeralLifetime* Get(content::BrowserContext* browser_context,
                                   const std::string& storage_domain);
  static scoped_refptr<TLDEphemeralLifetime> GetOrCreate(
      content::BrowserContext* browser_context,
      const std::string& storage_domain,
      const content::StoragePartitionConfig& storage_partition_config);

  // Add a callback to a callback list to be called on destruction.
  void RegisterOnDestroyCallback(OnDestroyCallback callback);

  const TLDEphemeralLifetimeKey& key() const { return key_; }
  const content::StoragePartitionConfig& storage_partition_config() const {
    return storage_partition_config_;
  }

 private:
  friend class RefCounted<TLDEphemeralLifetime>;
  virtual ~TLDEphemeralLifetime();

  static TLDEphemeralLifetime* Get(const TLDEphemeralLifetimeKey& key);

  TLDEphemeralLifetimeKey key_;
  base::WeakPtr<EphemeralStorageService> ephemeral_storage_service_;
  const content::StoragePartitionConfig storage_partition_config_;
  std::vector<OnDestroyCallback> on_destroy_callbacks_;

  base::WeakPtrFactory<TLDEphemeralLifetime> weak_factory_{this};
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_TLD_EPHEMERAL_LIFETIME_H_
