/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_STATUS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_STATUS_TASK_H_

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

namespace brave_wallet {

template <class R, class T>
std::optional<size_t> Position(R&& r, const T& val) {
  size_t i = 0;
  for (const auto& v : r) {
    if (v == val) {
      return i;
    }
    ++i;
  }
  return std::nullopt;
}

enum class PolkadotTransactionStatus {
  kSuccess,
  kFailed,
  kNotFound,
  kNotFinalized,
  kInvalidResponse,
};

class PolkadotTransactionStatusTask {
 public:
  using GetTransactionStatusCallback = base::OnceCallback<void(
      base::expected<std::pair<PolkadotTransactionStatus, uint128_t>,
                     std::string>)>;

  PolkadotTransactionStatusTask(PolkadotWalletService& polkadot_wallet_service,
                                KeyringService& keyring_service,
                                mojom::AccountIdPtr sender_account_id,
                                std::string chain_id,
                                std::vector<uint8_t> extrinsic,
                                uint32_t block_num,
                                uint32_t mortality_period);
  ~PolkadotTransactionStatusTask();

  void Start(GetTransactionStatusCallback callback);

 private:
  PolkadotSubstrateRpc* GetPolkadotRpc();

  void OnGetChainMetadata(
      base::expected<PolkadotChainMetadata, std::string> chain_metadata);

  void OnGetFinalizedHead(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
      std::optional<std::string> err_str);

  void OnGetFinalizedBlockHeader(
      std::optional<PolkadotBlockHeader> block_header,
      std::optional<std::string> err_str);

  void InitRequests();

  void OnGetBlockHashForStatus(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
      std::optional<std::string> err_str);

  void OnGetBlockForStatus(std::optional<PolkadotBlock> block,
                           std::optional<std::string> err_str);

  void OnGetEvents(base::expected<std::vector<uint8_t>, std::string> events);

  void HandleError(std::string err_str);

  base::raw_ref<PolkadotWalletService> polkadot_wallet_service_;
  base::raw_ref<KeyringService> keyring_service_;
  std::optional<PolkadotChainMetadata> chain_metadata_;

  mojom::AccountIdPtr sender_account_id_;
  std::string chain_id_;
  std::string extrinsic_hex_;
  uint32_t block_num_;
  uint32_t curr_block_num_;
  uint32_t mortality_period_;
  uint32_t finalized_block_num_ = 0;

  std::optional<size_t> extrinsic_idx_;
  GetTransactionStatusCallback callback_;

  base::WeakPtrFactory<PolkadotTransactionStatusTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_STATUS_TASK_H_
