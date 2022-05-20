/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class FilTransaction;

class FilecoinKeyring : public HDKeyring, public mojom::JsonRpcServiceObserver {
 public:
  explicit FilecoinKeyring(const std::string& chain_id);
  ~FilecoinKeyring() override;
  FilecoinKeyring(const FilecoinKeyring&) = delete;
  FilecoinKeyring& operator=(const FilecoinKeyring&) = delete;
  static bool DecodeImportPayload(const std::string& payload_hex,
                                  std::vector<uint8_t>* private_key_out,
                                  mojom::FilecoinAddressProtocol* protocol_out);
  std::string ImportFilecoinAccount(const std::vector<uint8_t>& private_key,
                                    const std::string& network,
                                    mojom::FilecoinAddressProtocol protocol);
  void RestoreFilecoinAccount(const std::vector<uint8_t>& input_key,
                              const std::string& address);
  mojo::PendingRemote<mojom::JsonRpcServiceObserver> GetReceiver() {
    return rpc_observer_receiver_.BindNewPipeAndPassRemote();
  }
  // mojom::JsonRpcServiceObserver
  void ChainChangedEvent(const std::string& chain_id,
                         mojom::CoinType coin) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override {}
  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {}

  absl::optional<std::string> SignTransaction(const FilTransaction* tx);

 private:
  std::string GetAddressInternal(HDKeyBase* hd_key_base) const override;
  std::string network_prefix_;
  mojo::Receiver<mojom::JsonRpcServiceObserver> rpc_observer_receiver_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FILECOIN_KEYRING_H_
