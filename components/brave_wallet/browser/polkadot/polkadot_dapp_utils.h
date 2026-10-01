/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_DAPP_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_DAPP_UTILS_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"

namespace brave_wallet {

inline constexpr char kPolkadotSr25519KeypairType[] = "sr25519";
inline constexpr char kPolkadotObfuscatedAccountName[] = "Brave Wallet Account";

class KeyringService;

// Identifiers of every Polkadot account, in the form the permission layer
// stores them. See `GetAccountPermissionIdentifier`.
std::vector<std::string> GetPolkadotAccountPermissionIdentifiers(
    KeyringService* keyring_service);

// The Polkadot account a dapp should be talking to, out of `allowed_accounts`.
// `allowed_accounts` is passed in rather than queried from the delegate so
// callers can use the list a permission request just resolved with, before the
// iOS front-end database has necessarily finished writing it.
mojom::AccountIdPtr GetPolkadotPreferredDappAccount(
    KeyringService* keyring_service,
    base::optional_ref<const std::vector<std::string>> allowed_accounts);

// Converts an account into the `InjectedAccount` shape dapps expect.
mojom::PolkadotInjectedAccountPtr MakePolkadotInjectedAccount(
    const mojom::AccountInfo& account);

// Whether `signature_payload` signs over the call the dapp declared in its
// `SignerPayloadJSON`. An `ExtrinsicPayload` encodes `method` first and bare,
// so the declared call is the payload's own prefix. The payload is built
// outside the browser, where a different call could be substituted for the one
// the user was shown, and this is what rules that out.
bool PolkadotSignaturePayloadMatchesCall(
    base::span<const uint8_t> signature_payload,
    std::string_view payload_json);

// A `SignerResult.signature`: the signature behind its `MultiSignature` variant
// index, hex-encoded. polkadot-js hands what a signer returns straight to
// `MultiSignature`, so a bare 64-byte signature would be read as the wrong
// variant.
std::string MakePolkadotSignerSignatureHex(
    base::span<const uint8_t, kSr25519SignatureSize> signature);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_DAPP_UTILS_H_
