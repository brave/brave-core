/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace brave_wallet {
class BitcoinTransactionDatabase;
class BitcoinDatabaseSynchronizer;
struct SendToContext;

class BitcoinWalletService : public KeyedService,
                             public mojom::BitcoinWalletService,
                             KeyringServiceObserverBase {
 public:
  BitcoinWalletService(
      KeyringService* keyring_service,
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BitcoinWalletService() override;

  mojo::PendingRemote<mojom::BitcoinWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BitcoinWalletService> receiver);

  void GetBalance(const std::string& network_id,
                  mojom::AccountIdPtr account_id,
                  GetBalanceCallback callback) override;

  void GetBitcoinAccountInfo(const std::string& network_id,
                             mojom::AccountIdPtr account_id,
                             GetBitcoinAccountInfoCallback callback) override;
  mojom::BitcoinAccountInfoPtr GetBitcoinAccountInfoSync(
      const std::string& network_id,
      mojom::AccountIdPtr account_id);

  void SendTo(const std::string& network_id,
              mojom::AccountIdPtr account_id,
              const std::string& address_to,
              uint64_t amount,
              uint64_t fee,
              SendToCallback callback) override;

  BitcoinRpc& bitcoin_rpc() { return *bitcoin_rpc_; }

 private:
  // KeyringServiceObserverBase:
  void AccountsAdded(std::vector<mojom::AccountInfoPtr> accounts) override;

  void StartDatabaseSynchronizer(const std::string& network_id,
                                 const mojom::AccountId& account_id);

  absl::optional<std::string> GetUnusedChangeAddress(
      const mojom::AccountId& account_id);

  bool FillUtxoList(SendToContext& context);
  bool PickInputs(SendToContext& context);
  bool PrepareOutputs(SendToContext& context);
  bool FillInputTransactions(std::unique_ptr<SendToContext> context);
  void OnFetchTransactionForSendTo(std::unique_ptr<SendToContext> context,
                                   std::string txid,
                                   base::Value transaction);
  bool FillSignature(SendToContext& context, uint32_t input_index);
  bool FillSignatures(SendToContext& context);
  bool SerializeTransaction(SendToContext& context);
  void PostTransaction(std::unique_ptr<SendToContext> context);
  void OnPostTransaction(std::unique_ptr<SendToContext> context,
                         base::expected<std::string, std::string> result);
  void WorkOnSendTo(std::unique_ptr<SendToContext> context);

  raw_ptr<KeyringService> keyring_service_;
  std::map<std::string, std::unique_ptr<BitcoinTransactionDatabase>>
      transaction_database_;
  std::map<std::string, std::unique_ptr<BitcoinDatabaseSynchronizer>>
      database_synchronizer_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojo::ReceiverSet<mojom::BitcoinWalletService> receivers_;
  std::unique_ptr<BitcoinRpc> bitcoin_rpc_;
  base::WeakPtrFactory<BitcoinWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_
