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
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"

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

  TLDEphemeralLifetime(const TLDEphemeralLifetimeKey& key,
                       StoragePartition* storage_partition);
  static TLDEphemeralLifetime* Get(BrowserContext* browser_context,
                                   const std::string& storage_domain);
  static scoped_refptr<TLDEphemeralLifetime> GetOrCreate(
      BrowserContext* browser_context,
      StoragePartition* storage_partition,
      const std::string& storage_domain);

  // Add a callback to a callback list to be called on destruction.
  void RegisterOnDestroyCallback(OnDestroyCallback callback);

  const TLDEphemeralLifetimeKey& key() { return key_; }

 private:
  friend class RefCounted<TLDEphemeralLifetime>;
  virtual ~TLDEphemeralLifetime();

  static TLDEphemeralLifetime* Get(const TLDEphemeralLifetimeKey& key);

  TLDEphemeralLifetimeKey key_;
  StoragePartition* storage_partition_;
  std::vector<OnDestroyCallback> on_destroy_callbacks_;

  base::WeakPtrFactory<TLDEphemeralLifetime> weak_factory_{this};
};

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_TLD_EPHEMERAL_LIFETIME_H_
