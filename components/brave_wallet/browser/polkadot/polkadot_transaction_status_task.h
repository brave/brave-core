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
};

using GetTransactionStatusCallback = base::OnceCallback<void(
    base::expected<std::pair<PolkadotTransactionStatus, uint128_t>,
                   std::string>)>;

class PolkadotTransactionStatusTask {
 public:
  PolkadotTransactionStatusTask(PolkadotWalletService& polkadot_wallet_service,
                                KeyringService& keyring_service,
                                mojom::AccountIdPtr sender_account_id,
                                std::string chain_id,
                                std::vector<uint8_t> extrinsic,
                                uint32_t block_num,
                                uint32_t mortality_period);
  ~PolkadotTransactionStatusTask();

  void Start(GetTransactionStatusCallback callback) {
    callback_ = std::move(callback);

    polkadot_wallet_service_->GetChainMetadata(
        chain_id_,
        base::BindOnce(&PolkadotTransactionStatusTask::OnGetChainMetadata,
                       weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  PolkadotSubstrateRpc* GetPolkadotRpc() {
    return polkadot_wallet_service_->GetPolkadotRpc();
  }

  void OnGetChainMetadata(
      base::expected<PolkadotChainMetadata, std::string> chain_metadata) {
    if (!chain_metadata.has_value()) {
      return std::move(callback_).Run(
          base::unexpected(WalletInternalErrorMessage()));
    }

    chain_metadata_ = std::move(chain_metadata.value());

    GetPolkadotRpc()->GetFinalizedHead(
        chain_id_,
        base::BindOnce(&PolkadotTransactionStatusTask::OnGetFinalizedHead,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void OnGetFinalizedHead(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
      std::optional<std::string> err_str) {
    if (err_str) {
      return HandleError(std::move(*err_str));
    }

    if (!block_hash) {
      return HandleError(WalletInternalErrorMessage());
    }

    GetPolkadotRpc()->GetBlockHeader(
        chain_id_, *block_hash,
        base::BindOnce(
            &PolkadotTransactionStatusTask::OnGetFinalizedBlockHeader,
            weak_ptr_factory_.GetWeakPtr()));
  }

  void OnGetFinalizedBlockHeader(
      std::optional<PolkadotBlockHeader> block_header,
      std::optional<std::string> err_str) {
    if (err_str) {
      return HandleError(std::move(*err_str));
    }

    if (!block_header) {
      return HandleError(WalletInternalErrorMessage());
    }

    finalized_block_num_ = block_header->block_number;
    InitRequests();
  }

  void InitRequests() {
    if (curr_block_num_ > finalized_block_num_) {
      return std::move(callback_).Run(
          base::ok(std::pair(PolkadotTransactionStatus::kNotFinalized, 0)));
    }

    if (curr_block_num_ >= (block_num_ + mortality_period_)) {
      return std::move(callback_).Run(
          base::ok(std::pair(PolkadotTransactionStatus::kNotFound, 0)));
    }

    GetPolkadotRpc()->GetBlockHash(
        chain_id_, curr_block_num_,
        base::BindOnce(&PolkadotTransactionStatusTask::OnGetBlockHashForStatus,
                       weak_ptr_factory_.GetWeakPtr()));

    ++curr_block_num_;
  }

  void OnGetBlockHashForStatus(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
      std::optional<std::string> err_str) {
    if (err_str) {
      return HandleError(std::move(*err_str));
    }

    if (!block_hash) {
      return HandleError(WalletInternalErrorMessage());
    }

    GetPolkadotRpc()->GetBlock(
        chain_id_, block_hash.value(),
        base::BindOnce(&PolkadotTransactionStatusTask::OnGetBlockForStatus,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void OnGetBlockForStatus(std::optional<PolkadotBlock> block,
                           std::optional<std::string> err_str) {
    if (err_str) {
      return HandleError(std::move(*err_str));
    }

    if (!block) {
      return HandleError(WalletInternalErrorMessage());
    }

    if (extrinsic_idx_.has_value()) {
      return;
    }

    auto idx = Position(block->extrinsics, extrinsic_hex_);
    if (idx.has_value()) {
      extrinsic_idx_ = idx;
      GetPolkadotRpc()->GetEvents(
          chain_id_, block->header.GetHash(),
          base::BindOnce(&PolkadotTransactionStatusTask::OnGetEvents,
                         weak_ptr_factory_.GetWeakPtr()));

      return;
    }

    InitRequests();
  }

  void OnGetEvents(base::expected<std::vector<uint8_t>, std::string> events) {
    if (!events.has_value()) {
      return HandleError(std::move(events.error()));
    }

    auto pubkey = keyring_service_->GetPolkadotPubKey(sender_account_id_);
    CHECK(pubkey);

    std::array<uint8_t, 16> fee = {};

    bool was_successful = was_extrinsic_successful(
        ::rust::Slice<const uint8_t>(events.value()), *extrinsic_idx_, *pubkey,
        **chain_metadata_, fee);

    std::move(callback_).Run(
        base::ok(std::pair(was_successful ? PolkadotTransactionStatus::kSuccess
                                          : PolkadotTransactionStatus::kFailed,
                           base::bit_cast<uint128_t>(fee))));
  }

  void HandleError(std::string err_str) {
    std::move(callback_).Run(base::unexpected(std::move(err_str)));
  }

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
