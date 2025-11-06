/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

PolkadotWalletService::PolkadotWalletService(
    KeyringService& keyring_service,
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      polkadot_substrate_rpc_(network_manager, std::move(url_loader_factory)) {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

PolkadotWalletService::~PolkadotWalletService() = default;

void PolkadotWalletService::Bind(
    mojo::PendingReceiver<mojom::PolkadotWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void PolkadotWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void PolkadotWalletService::GetNetworkName(mojom::AccountIdPtr account_id,
                                           GetNetworkNameCallback callback) {
  std::string chain_id = GetNetworkForPolkadotAccount(account_id);
  polkadot_substrate_rpc_.GetChainName(std::move(chain_id),
                                       std::move(callback));
}

void PolkadotWalletService::GetAccountBalance(
    mojom::AccountIdPtr account_id,
    const std::string& chain_id,
    GetAccountBalanceCallback callback) {
  // auto pubkey = keyring_service_->GetPolkadotPubKey(account_id);
  // if (!pubkey) {
  //   return std::move(callback).Run(
  //       nullptr, "Cannot get Polkadot public key for this account.");
  // }

  auto pubkey = keyring_service_->GetPolkadotPubKey(account_id);
  LOG(INFO) << "cxx generated pubkey: 0x" << base::HexEncode(*pubkey);

  polkadot_remote_->GetPublicKey(
      std::move(account_id), base::BindOnce(&PolkadotWalletService::OnGetPubKey,
                                            weak_ptr_factory_.GetWeakPtr(),
                                            chain_id, std::move(callback)));
}

void PolkadotWalletService::AddObserver(
    mojo::PendingRemote<mojom::PolkadotWalletServiceObserver> observer) {
  polkadot_remote_.Bind(std::move(observer));
}

std::string PolkadotWalletService::GetPubKey() {
  return "";
}

void PolkadotWalletService::OnGetPubKey(std::string chain_id,
                                        GetAccountBalanceCallback callback,
                                        const std::string& pubkey) {
  LOG(INFO) << "received the pubkey from the polkadot-bridge webui: " << pubkey;

  std::vector<uint8_t> bytes;
  base::HexStringToBytes(pubkey, &bytes);

  polkadot_substrate_rpc_.GetAccountBalance(
      chain_id,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize>(bytes),
      std::move(callback));
}

}  // namespace brave_wallet
