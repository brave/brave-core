/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_TLD_EPHEMERAL_LIFETIME_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_TLD_EPHEMERAL_LIFETIME_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "content/public/browser/storage_partition_config.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace content {

class BrowserContext;

}  // namespace content

namespace ephemeral_storage {

// TLD storage is keyed by the BrowserContext (profile) and the TLD-specific
// security domain.
struct TLDEphemeralLifetimeKey {
  raw_ptr<content::BrowserContext> browser_context;
  std::string storage_domain;
  content::StoragePartitionConfig storage_partition_config;

  auto operator<=>(const TLDEphemeralLifetimeKey& other) const {
    auto tie = [](const TLDEphemeralLifetimeKey& key) {
      return std::tie(key.browser_context, key.storage_domain,
                      key.storage_partition_config);
    };
    return tie(*this) <=> tie(other);
  }
};

// This class is responsible for managing the lifetime of ephemeral storage
// cookies. Each instance is shared by each top-level frame with the same
// TLDEphemeralLifetimeKey. When the last top-level frame holding a reference
// is destroyed or navigates to a new storage domain, storage will be
// cleared.
class TLDEphemeralLifetime : public base::RefCounted<TLDEphemeralLifetime> {
 public:
  using OnDestroyCallback = base::OnceCallback<void(const std::string&)>;

  explicit TLDEphemeralLifetime(const TLDEphemeralLifetimeKey& key);
  static TLDEphemeralLifetime* Get(const TLDEphemeralLifetimeKey& key);
  static scoped_refptr<TLDEphemeralLifetime> GetOrCreate(
      const TLDEphemeralLifetimeKey& key);

  const TLDEphemeralLifetimeKey& key() const { return key_; }
  void SetShieldsStateOnHost(std::string_view host, bool enabled);
  void EnforceFirstPartyStorageCleanup();

 private:
  friend class RefCounted<TLDEphemeralLifetime>;
  virtual ~TLDEphemeralLifetime();

  TLDEphemeralLifetimeKey key_;
  base::WeakPtr<EphemeralStorageService> ephemeral_storage_service_;
  absl::flat_hash_map<std::string, bool> shields_state_on_hosts_;
  bool first_party_storage_cleanup_enforced_{false};

  base::WeakPtrFactory<TLDEphemeralLifetime> weak_factory_{this};
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_TLD_EPHEMERAL_LIFETIME_H_
