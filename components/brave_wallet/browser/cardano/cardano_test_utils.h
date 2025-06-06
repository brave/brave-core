/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TEST_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_blockfrost_api.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "services/network/test/test_url_loader_factory.h"

namespace brave_wallet {

class CardanoWalletService;

inline constexpr char kMockCardanoAddress1[] =
    "addr1q9zwt6rfn2e3mc63hesal6muyg807cwjnkwg3j5azkvmxm0tyqeyc8eu034zzmj4z53l7"
    "lh5u7z08l0rvp49ht88s5uskl6tsl";

inline constexpr char kMockCardanoAddress2[] =
    "addr1q8s90ehlgwwkq637d3r6qzuxwu6qnprphqadn9pjg2mtcp9hkfmyv4zfhyefvjmpww7f7"
    "w9gwem3x6gcm3ulw3kpcgws9sgrhg";

inline constexpr char kMockCardanoTxid[] =
    "7e2aeed860faf61b0513e9807be633a90e3260480ebc46b53ea99c497195fc29";

class CardanoTestRpcServer {
 public:
  explicit CardanoTestRpcServer(CardanoWalletService& cardano_wallet_service);
  ~CardanoTestRpcServer();

  void SetUpCardanoRpc(const std::optional<std::string>& mnemonic,
                       std::optional<uint32_t> account_index);

  void AddUtxo(const std::string& address, uint32_t amount);

  void FailNextTransactionSubmission();
  void ConfirmAllTransactions();
  void AddConfirmedTransaction(const std::string& txid);

  std::string captured_raw_tx() const { return captured_raw_tx_; }

  void set_fail_latest_epoch_parameters_request(bool value) {
    fail_latest_epoch_parameters_request_ = value;
  }
  void set_fail_latest_block_request(bool value) {
    fail_latest_block_request_ = value;
  }
  void set_fail_address_utxo_request(bool value) {
    fail_address_utxo_request_ = value;
  }

  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory();

 private:
  void RequestInterceptor(const network::ResourceRequest& request);
  std::string ExtractApiRequestPath(const GURL& request_url);
  std::optional<std::string> IsAddressUtxoRequest(
      const network::ResourceRequest& request);
  bool IsLatestEpochParametersRequest(const network::ResourceRequest& request);
  bool IsLatestBlockRequest(const network::ResourceRequest& request);
  bool IsTxSubmitRequest(const network::ResourceRequest& request);
  std::optional<std::string> IsGetTransactionRequest(
      const network::ResourceRequest& request);
  std::array<uint8_t, 32> CreateNewTxHash();

  std::map<std::string, std::vector<cardano_rpc::blockfrost_api::UnspentOutput>>
      utxos_map_;
  std::array<uint8_t, 32> next_tx_hash_ = {};

  std::string captured_raw_tx_;
  bool fail_next_transaction_submission_ = false;
  std::vector<std::string> mempool_transactions_;
  std::vector<std::string> confirmed_transactions_;

  bool fail_latest_epoch_parameters_request_ = false;
  bool fail_latest_block_request_ = false;
  bool fail_address_utxo_request_ = false;

  network::TestURLLoaderFactory url_loader_factory_;
  raw_ref<CardanoWalletService> cardano_wallet_service_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<CardanoHDKeyring> keyring_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TEST_UTILS_H_
