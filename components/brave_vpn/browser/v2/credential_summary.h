/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_SUMMARY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_SUMMARY_H_

#include <optional>
#include <string_view>

namespace brave_vpn::v2 {

// Parsed SKUS credential-summary response. This is a pure value type with no
// dependencies: it describes the server's view of the subscription, which is a
// different concept from the credential cached on disk.
class CredentialSummary final {
 public:
  // Parses a raw summary message.
  // Returns std::nullopt if the body was non-empty but not valid JSON (a
  // genuine parse failure). Returns an empty summary if the body was
  // empty/whitespace-only, or parsed to an empty object (no subscription on
  // record). Otherwise returns a populated summary; inspect the predicates.
  static std::optional<CredentialSummary> FromMessage(std::string_view message);

  bool IsEmpty() const { return empty_; }
  bool IsValid() const { return active_ && remaining_credential_count_ > 0; }
  bool NeedsActivation() const {
    return !active_ && remaining_credential_count_ > 0;
  }

 private:
  CredentialSummary() = default;

  // True when the body was empty or an empty JSON object: SKUS has no
  // subscription on record for this domain at all.
  bool empty_ = true;

  // Whether the subscription's credentials have been activated for this
  // customer. An inactive subscription with credentials remaining means the
  // user purchased but the credentials still need online activation.
  bool active_ = false;

  // How many credentials remain redeemable for this subscription. Zero with a
  // subscription on record means the purchase is expired.
  int remaining_credential_count_ = 0;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_SUMMARY_H_
