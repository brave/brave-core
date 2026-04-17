/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_STATUS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_STATUS_TASK_H_

#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_header.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

class KeyringService;
class PolkadotWalletService;
class PolkadotSubstrateRpc;

enum class PolkadotTransactionStatus {
  // Found the extrinsic, and it succeeded.
  kSuccess,
  // Found the extrinsic, and it failed.
  kFailed,
  // Not found within the entire mortality window.
  kNotFound,
  // Not found, finalized block was within mortality window.
  kNotFinalized,
  // Found the extrinsic, but events data was corrupted.
  kInvalidResponse,
};

// Used to probe the provided chain for an extrinsic and its status. Searching
// encompasses the range: [block_num, block_num + mortality_period). This is
// because in Polkadot, an extrinsic is tied to a specific signing block and
// from there is guaranteed to be on-chain within mortality_period blocks
// relative to the signing block.
// At a maximum performs mortality_period block fetches.
// Only finalized blocks are considered.
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

  void FetchCurrentBlock();

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
  uint32_t block_num_ = 0;
  uint32_t mortality_period_ = 0;
  uint32_t max_block_num_ = 0;
  uint32_t finalized_block_num_ = 0;
  uint32_t curr_block_num_ = 0;

  std::optional<size_t> extrinsic_idx_;
  GetTransactionStatusCallback callback_;

  base::WeakPtrFactory<PolkadotTransactionStatusTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_STATUS_TASK_H_
