/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_STORE_H_

#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/time/time.h"

class PrefService;

namespace brave_vpn::v2 {

// CredentialStore owns the on-disk credential slot used by VPN purchase flow.
// The slot is a single pref dictionary that, depending on where we are in the
// credential chain, holds *either* a SKUS credential awaiting exchange *or* the
// subscriber credential it was exchanged for, but never both, and both share
// one expiration entry. Because the two occupy one slot, each SetXXXCredential
// rewrites the entire dictionary, and so drops the other credential.
class CredentialStore final {
 public:
  explicit CredentialStore(PrefService* local_prefs);
  ~CredentialStore();

  CredentialStore(const CredentialStore&) = delete;
  CredentialStore& operator=(const CredentialStore&) = delete;

  // Subscriber credential: the ready-to-use credential.
  bool HasValidSubscriberCredential() const;
  std::string GetSubscriberCredential() const;

  // Replaces the slot with this subscriber credential, dropping any cached
  // SKUS credential.
  void SetSubscriberCredential(const std::string& credential,
                               base::Time expiration);

  // SKUS credential: obtained from the presentation cookie, awaiting exchange
  // for a subscriber credential.
  bool HasValidSkusCredential() const;
  std::string GetSkusCredential() const;

  // Replaces the slot with this SKUS credential (dropping any cached subscriber
  // credential) and stamps the last credential expiry.
  void SetSkusCredential(const std::string& credential, base::Time expiration);

  // True if anything at all is cached, valid or not.
  bool HasAnyCredential() const;

  // Expiration of the currently cached subscriber credential, used to drive the
  // refresh timer. Returns nullopt unless a valid subscriber credential is
  // present.
  std::optional<base::Time> GetExpirationTime() const;

  // Drops the cached credential only. Delegates to the common utility
  // ClearSubscriberCredential() so other existing callers won't drift.
  void Clear();

  // Exchange-retry guard needed to stop the re-exchange loop within a single
  // resolution cycle, which is inherently per-session, so it lives in memory
  // and is intentionally not persisted.
  bool IsExchangeRetried() const { return exchange_retried_; }
  void SetExchangeRetried(bool retried) { exchange_retried_ = retried; }

 private:
  const raw_ref<PrefService> local_prefs_;
  bool exchange_retried_ = false;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_STORE_H_
