/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_TLD_EPHEMERAL_LIFETIME_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_TLD_EPHEMERAL_LIFETIME_H_

#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "url/origin.h"

namespace content {

class BrowserContext;
class SessionStorageNamespace;
class StoragePartition;
class StoragePartition;
class WebContents;

// TLD storage is keyed by the BrowserContext (profile) and the TLD-specific
// security domain.
using TLDEphemeralLifetimeKey =
    std::pair<content::BrowserContext*, std::string>;

// This class is responsible for managing the lifetime of ephemeral storage
// cookies. Each instance is shared by each top-level frame with the same
// TLDEphemeralLifetimeKey. When the last top-level frame holding a reference
// is destroyed or navigates to a new storage domain, storage will be
// cleared.
//
// TODO(mrobinson): Have this class also manage ephemeral local storage and
// take care of handing out new instances of session storage.
class CONTENT_EXPORT TLDEphemeralLifetime
    : public base::RefCounted<TLDEphemeralLifetime> {
 public:
  using OnDestroyCallback = base::OnceCallback<void(const std::string&)>;

  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Should return opaque origins which were used for keying ephemeral
    // storages during the ephemeral TLD lifetime. These origins are used to
    // cleanup storages.
    virtual std::vector<url::Origin> TakeEphemeralStorageOpaqueOrigins(
        const std::string& ephemeral_storage_domain) = 0;
  };

  TLDEphemeralLifetime(const TLDEphemeralLifetimeKey& key,
                       StoragePartition* storage_partition,
                       std::unique_ptr<Delegate> delegate);
  static TLDEphemeralLifetime* Get(BrowserContext* browser_context,
                                   const std::string& storage_domain);
  static scoped_refptr<TLDEphemeralLifetime> GetOrCreate(
      BrowserContext* browser_context,
      StoragePartition* storage_partition,
      const std::string& storage_domain,
      std::unique_ptr<Delegate> delegate);

  // Add a callback to a callback list to be called on destruction.
  void RegisterOnDestroyCallback(OnDestroyCallback callback);

  const TLDEphemeralLifetimeKey& key() const { return key_; }

 private:
  friend class RefCounted<TLDEphemeralLifetime>;
  virtual ~TLDEphemeralLifetime();

  static TLDEphemeralLifetime* Get(const TLDEphemeralLifetimeKey& key);

  TLDEphemeralLifetimeKey key_;
  raw_ptr<StoragePartition> storage_partition_ = nullptr;
  std::unique_ptr<Delegate> delegate_;
  std::vector<OnDestroyCallback> on_destroy_callbacks_;

  base::WeakPtrFactory<TLDEphemeralLifetime> weak_factory_{this};
};

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_TLD_EPHEMERAL_LIFETIME_H_
