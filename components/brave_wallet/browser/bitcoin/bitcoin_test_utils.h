/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TEST_UTILS_H_

#include <map>
#include <string>

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"
#include "brave/components/brave_wallet/browser/bitcoin_rpc_responses.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "services/network/test/test_url_loader_factory.h"

namespace brave_wallet {

const char kMockBtcTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
const char kMockBtcTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
const char kMockBtcTxid3[] =
    "f4024cb219b898ed51a5c2a2d0589c1de4bb35e329ad15ab08b6ac9ffcc95ae2";
const char kMockBtcAddress[] = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";

class BitcoinTestRpcServer {
 public:
  BitcoinTestRpcServer(KeyringService* keyring_service, PrefService* prefs);
  ~BitcoinTestRpcServer();

  static bitcoin_rpc::AddressStats EmptyAddressStats(
      const std::string& address);

  static bitcoin_rpc::AddressStats TransactedAddressStats(
      const std::string& address);

  static bitcoin_rpc::AddressStats MempoolAddressStats(
      const std::string& address,
      uint64_t funded,
      uint64_t spent);

  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory();

  void SetUpBitcoinRpc(const mojom::AccountIdPtr& account_id);
  void AddTransactedAddress(const std::string& address);
  void AddMempoolBalance(const std::string& address,
                         uint64_t funded,
                         uint64_t spent);

  const std::string& Address0() const { return address_0_; }
  const std::string& Address6() const { return address_6_; }

  std::map<std::string, bitcoin_rpc::AddressStats>& address_stats_map() {
    return address_stats_map_;
  }

  const std::string& captured_raw_tx() { return captured_raw_tx_; }

 protected:
  void RequestInterceptor(const network::ResourceRequest& request);

 private:
  std::string mainnet_rpc_url_ = "https://btc-mainnet.com/";
  std::string testnet_rpc_url_ = "https://btc-testnet.com/";
  uint32_t mainnet_height_ = 12345;
  std::string address_0_;
  std::string address_6_;
  std::map<std::string, bitcoin_rpc::AddressStats> address_stats_map_;
  std::map<std::string, bitcoin_rpc::UnspentOutputs> utxos_map_;
  base::Value fee_estimates_;
  std::string captured_raw_tx_;

  mojom::AccountIdPtr account_id_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<PrefService> prefs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TEST_UTILS_H_
