/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_FIRST_PARTY_STORAGE_LIFETIME_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_FIRST_PARTY_STORAGE_LIFETIME_H_

#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ephemeral_storage {

using FirstPartyStorageLifetimeKey =
    std::pair<content::BrowserContext*, url::Origin>;

class FirstPartyStorageLifetime
    : public base::RefCounted<FirstPartyStorageLifetime> {
 public:
  explicit FirstPartyStorageLifetime(const FirstPartyStorageLifetimeKey& key);
  static FirstPartyStorageLifetime* Get(
      content::BrowserContext* browser_context,
      const url::Origin& origin);
  static scoped_refptr<FirstPartyStorageLifetime> GetOrCreate(
      content::BrowserContext* browser_context,
      const url::Origin& origin);

  const FirstPartyStorageLifetimeKey& key() const { return key_; }

 private:
  friend class RefCounted<FirstPartyStorageLifetime>;
  virtual ~FirstPartyStorageLifetime();

  static FirstPartyStorageLifetime* Get(
      const FirstPartyStorageLifetimeKey& key);

  const FirstPartyStorageLifetimeKey key_;
  base::WeakPtr<EphemeralStorageService> ephemeral_storage_service_;

  base::WeakPtrFactory<FirstPartyStorageLifetime> weak_factory_{this};
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_FIRST_PARTY_STORAGE_LIFETIME_H_
