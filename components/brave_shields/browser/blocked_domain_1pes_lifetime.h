/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BLOCKED_DOMAIN_1PES_LIFETIME_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BLOCKED_DOMAIN_1PES_LIFETIME_H_

#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace ephemeral_storage {
class EphemeralStorageService;
}  // namespace ephemeral_storage

namespace brave_shields {

// Manages the lifetime of auto-enabled 1PES mode (by DomainBlock feature).
// Each instance is shared by each 1PES-enabled top-level frame with the same
// BlockedDomain1PESLifetime::Key. When the last top-level frame holding a
// reference is destroyed or navigates to a non-blocked domain, 1PES will be
// disabled.
class BlockedDomain1PESLifetime
    : public base::RefCounted<BlockedDomain1PESLifetime>,
      public base::SupportsWeakPtr<BlockedDomain1PESLifetime> {
 public:
  using Key = std::pair<ephemeral_storage::EphemeralStorageService*, GURL>;

  static scoped_refptr<BlockedDomain1PESLifetime> GetOrCreate(
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      const GURL& url);

  explicit BlockedDomain1PESLifetime(const Key& Key);

  void AddOnReadyCallback(base::OnceCallback<void()> on_ready);

 private:
  friend class RefCounted<BlockedDomain1PESLifetime>;
  virtual ~BlockedDomain1PESLifetime();

  void Start1PESEnableRequest();
  void On1PESEnableRequestComplete(bool is_enabled);

  const Key key_;
  std::vector<base::OnceCallback<void()>> on_ready_;
  absl::optional<bool> is_1pes_enabled_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BLOCKED_DOMAIN_1PES_LIFETIME_H_
