/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TEST_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_hd_keyring.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/bitcoin_rpc_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "services/network/test/test_url_loader_factory.h"

namespace brave_wallet {

class BitcoinWalletService;

inline constexpr char kMockBtcTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
inline constexpr char kMockBtcTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
inline constexpr char kMockBtcTxid3[] =
    "f4024cb219b898ed51a5c2a2d0589c1de4bb35e329ad15ab08b6ac9ffcc95ae2";
inline constexpr char kMockBtcAddress[] =
    "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";

// Accounts generated from kMnemonicAbandonAbandon mnemonic.
inline constexpr char kBtcMainnetImportAccount0[] =
    "zprvAdG4iTXWBoARxkkzNpNh8r6Qag3irQB8PzEMkAFeTRXxHpbF9z4QgEvBRmfvqWvGp42t42"
    "nvgGpNgYSJA9iefm1yYNZKEm7z6qUWCroSQnE";  // m/84'/0'/0'
inline constexpr char kBtcMainnetImportAccount1[] =
    "zprvAdG4iTXWBoAS2cCGuaGevCvH54GCunrvLJb2hoWCSuE3D9LS42XVg3c6sPm64w6VMq3w18"
    "vJf8nF3cBA2kUMkyWHsq6enWVXivzw42UrVHG";  // m/84'/0'/1'
inline constexpr char kBtcTestnetImportAccount0[] =
    "vprv9K7GLAaERuM58PVvbk1sMo7wzVCoPwzZpVXLRBmum93gL5pSqQCAAvZjtmz93nnnYMr9i2"
    "FwG2fqrwYLRgJmDDwFjGiamGsbRMJ5Y6siJ8H";  // m/84'/1'/0'
inline constexpr char kBtcTestnetImportAccount1[] =
    "vprv9K7GLAaERuM5CAKPEd5qaDFXn67e95YPxcSUXpD7A1dvei4bQLCuH8DDz2RjtR5bS6nHyo"
    "SXbaMZ2K2DzVUrZ9SAYjwuZV39iTyRsiQG7N9";  // m/84'/1'/1'
inline constexpr char kBtcMainnetHardwareAccount0[] =
    "xpub6CatWdiZiodmUeTDp8LT5or8nmbKNcuyvz7WyksVFkKB4RHwCD3XyuvPEbvqAQY3rAPshW"
    "cMLoP2fMFMKHPJ4ZeZXYVUhLv1VMrjPC7PW6V";  // m/84'/0'/0'
inline constexpr char kBtcMainnetHardwareAccount1[] =
    "xpub6CatWdiZiodmYVtWLtEQsAg1H9ooS1bmsJUBwQ83FE1Fyk386FWcyicJgEZv3quZSJKA5d"
    "h5Lo2PbubMGxCfZtRthV6ST2qquL9w3HSzcUn";  // m/84'/0'/1'
inline constexpr char kBtcTestnetHardwareAccount0[] =
    "tpubDC8msFGeGuwnKG9Upg7DM2b4DaRqg3CUZa5g8v2SRQ6K4NSkxUgd7HsL2XVWbVm39yBA4L"
    "AxysQAm397zwQSQoQgewGiYZqrA9DsP4zbQ1M";  // m/84'/1'/0'
inline constexpr char kBtcTestnetHardwareAccount1[] =
    "tpubDC8msFGeGuwnP2xwTZBBZSie1BLgRAkJhgzpFYTdpGgZNzguXQhNDVWp7mJbHJUjQQvV2m"
    "yLU9dkx67a7VAUnzY7yT7nvhHj7FgS4oNivvq";  // m/84'/1'/1'

class BitcoinTestRpcServer {
 public:
  BitcoinTestRpcServer();
  explicit BitcoinTestRpcServer(BitcoinWalletService* bitcoin_wallet_service);
  ~BitcoinTestRpcServer();

  static bitcoin_rpc::AddressStats EmptyAddressStats(
      const std::string& address);

  static bitcoin_rpc::AddressStats TransactedAddressStats(
      const std::string& address);

  static bitcoin_rpc::AddressStats BalanceAddressStats(
      const std::string& address,
      uint64_t balance);

  static bitcoin_rpc::AddressStats MempoolAddressStats(
      const std::string& address,
      uint64_t funded,
      uint64_t spent);

  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory();

  void SetUpBitcoinRpc(const std::optional<std::string>& mnemonic,
                       std::optional<uint32_t> account_index);
  void AddTransactedAddress(const mojom::BitcoinAddressPtr& address);
  void AddBalanceAddress(const mojom::BitcoinAddressPtr& address,
                         uint64_t balance);
  void AddMempoolBalance(const mojom::BitcoinAddressPtr& address,
                         uint64_t funded,
                         uint64_t spent);

  void FailNextTransactionBroadcast();
  void ConfirmAllTransactions();

  const mojom::BitcoinAddressPtr& Address0() const { return address_0_; }
  const mojom::BitcoinAddressPtr& Address6() const { return address_6_; }

  std::map<std::string, bitcoin_rpc::AddressStats>& address_stats_map() {
    return address_stats_map_;
  }

  const std::string& captured_raw_tx() { return captured_raw_tx_; }

 protected:
  void RequestInterceptor(const network::ResourceRequest& request);

 private:
  uint32_t mainnet_height_ = 12345;
  mojom::BitcoinAddressPtr address_0_;
  mojom::BitcoinAddressPtr address_6_;
  std::map<std::string, bitcoin_rpc::AddressStats> address_stats_map_;
  std::map<std::string, bitcoin_rpc::UnspentOutputs> utxos_map_;
  base::Value fee_estimates_;
  std::string captured_raw_tx_;
  bool fail_next_transaction_broadcast_ = false;
  std::vector<bitcoin_rpc::Transaction> broadcasted_transactions_;

  std::optional<uint32_t> account_index_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<BitcoinHDKeyring> keyring_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TEST_UTILS_H_
