/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/external_wallet/external_wallet_data.h"

#include <string>

#include "bat/ledger/global_constants.h"

namespace ledger {

namespace {

bool IsMojoWalletActive(const mojom::ExternalWallet& wallet) {
  switch (wallet.status) {
    case mojom::WalletStatus::NOT_CONNECTED:
    case mojom::WalletStatus::CONNECTED:
    case mojom::WalletStatus::DISCONNECTED_NOT_VERIFIED:
    case mojom::WalletStatus::PENDING:
      return false;
    case mojom::WalletStatus::VERIFIED:
    case mojom::WalletStatus::DISCONNECTED_VERIFIED:
      return true;
  }
}

}  // namespace

std::string StringifyEnum(ExternalWalletProvider value) {
  switch (value) {
    case ExternalWalletProvider::kUphold:
      return "uphold";
    case ExternalWalletProvider::kGemini:
      return "gemini";
    case ExternalWalletProvider::kBitflyer:
      return "bitflyer";
  }
}

absl::optional<ExternalWalletProvider> ParseEnum(
    const EnumString<ExternalWalletProvider>& s) {
  return s.Match({ExternalWalletProvider::kUphold,
                  ExternalWalletProvider::kGemini,
                  ExternalWalletProvider::kBitflyer});
}

absl::optional<ExternalWallet> ExternalWalletFromMojoStruct(
    const mojom::ExternalWallet& wallet) {
  if (!IsMojoWalletActive(wallet)) {
    return {};
  }

  auto provider = EnumString<ExternalWalletProvider>::Parse(wallet.type);
  if (!provider) {
    return {};
  }

  bool connected = wallet.status == mojom::WalletStatus::VERIFIED;

  ExternalWallet external_wallet;
  external_wallet.provider = *provider;
  external_wallet.address = wallet.address;
  external_wallet.access_token = connected ? wallet.token : "";

  return external_wallet;
}

}  // namespace ledger
