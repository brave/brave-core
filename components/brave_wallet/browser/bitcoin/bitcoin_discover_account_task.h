/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_DISCOVER_ACCOUNT_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_DISCOVER_ACCOUNT_TASK_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class BitcoinWalletService;
class HDKey;

struct DiscoveredBitcoinAccount {
  DiscoveredBitcoinAccount();
  ~DiscoveredBitcoinAccount();
  DiscoveredBitcoinAccount(DiscoveredBitcoinAccount&& other);

  uint32_t next_unused_receive_index = 0;
  uint32_t next_unused_change_index = 0;
  mojom::BitcoinBalancePtr balance;

  bool operator==(const DiscoveredBitcoinAccount& other) const;
};

class DiscoverAccountTaskBase {
 public:
  using DiscoverAccountCallback = base::OnceCallback<void(
      base::expected<DiscoveredBitcoinAccount, std::string>)>;

  DiscoverAccountTaskBase(BitcoinWalletService& bitcoin_wallet_service,
                          const std::string& network_id);
  virtual ~DiscoverAccountTaskBase();

  void ScheduleWorkOnTask();
  void set_callback(DiscoverAccountCallback callback) {
    callback_ = std::move(callback);
  }

 protected:
  struct State {
    mojom::BitcoinAddressPtr last_transacted_address;
    mojom::BitcoinAddressPtr last_requested_address;
    State();
    ~State();
  };

  virtual mojom::BitcoinAddressPtr GetAddressById(
      const mojom::BitcoinKeyIdPtr& key_id) = 0;

  bool MaybeQueueRequests(bool receive_state);

  void WorkOnTask();
  void OnGetAddressStats(
      bool receive_state,
      mojom::BitcoinAddressPtr address,
      base::expected<bitcoin_rpc::AddressStats, std::string> stats);

  BitcoinWalletService& bitcoin_wallet_service() {
    return *bitcoin_wallet_service_;
  }

 private:
  State& GetState(bool receive_state);

  const raw_ref<BitcoinWalletService> bitcoin_wallet_service_;
  std::string network_id_;

  uint32_t active_requests_ = 0;
  State receive_addresses_state_;
  State change_addresses_state_;
  bool account_is_used_ = false;
  mojom::BitcoinBalancePtr balance_;

  std::optional<std::string> error_;
  DiscoverAccountCallback callback_;
  base::WeakPtrFactory<DiscoverAccountTaskBase> weak_ptr_factory_{this};
};

class DiscoverWalletAccountTask : public DiscoverAccountTaskBase {
 public:
  DiscoverWalletAccountTask(BitcoinWalletService& bitcoin_wallet_service,
                            mojom::KeyringId keyring_id,
                            uint32_t account_index);
  ~DiscoverWalletAccountTask() override;

 private:
  mojom::BitcoinAddressPtr GetAddressById(
      const mojom::BitcoinKeyIdPtr& key_id) override;
  mojom::KeyringId keyring_id_;
  uint32_t account_index_ = 0;
};

class DiscoverExtendedKeyAccountTask : public DiscoverAccountTaskBase {
 public:
  DiscoverExtendedKeyAccountTask(BitcoinWalletService& bitcoin_wallet_service,
                                 const std::string& network_id,
                                 const std::string& extended_key);
  ~DiscoverExtendedKeyAccountTask() override;

 private:
  mojom::BitcoinAddressPtr GetAddressById(
      const mojom::BitcoinKeyIdPtr& key_id) override;

  bool testnet_ = false;
  std::unique_ptr<HDKey> account_key_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_DISCOVER_ACCOUNT_TASK_H_
