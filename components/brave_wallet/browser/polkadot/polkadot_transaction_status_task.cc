/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_transaction_status_task.h"

#include "base/strings/strcat.h"  // IWYU pragma: keep
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

namespace brave_wallet {

template <class R, class T>
std::optional<size_t> FindPosition(R&& r, const T& val) {
  size_t i = 0;
  for (const auto& v : r) {
    if (v == val) {
      return i;
    }
    ++i;
  }
  return std::nullopt;
}

std::unique_ptr<PolkadotTransactionStatusTask>
PolkadotTransactionStatusTask::Create(
    PolkadotWalletService& polkadot_wallet_service,
    KeyringService& keyring_service,
    mojom::AccountIdPtr sender_account_id,
    std::string chain_id,
    base::span<const uint8_t> extrinsic,
    uint32_t block_num,
    uint32_t mortality_period) {
  static constexpr uint32_t kMaxMortalityPeriod = 1024;
  if (mortality_period > kMaxMortalityPeriod) {
    return nullptr;
  }

  return std::make_unique<PolkadotTransactionStatusTask>(
      PassKey{}, polkadot_wallet_service, keyring_service,
      std::move(sender_account_id), std::move(chain_id), std::move(extrinsic),
      block_num, mortality_period);
}

PolkadotTransactionStatusTask::PolkadotTransactionStatusTask(
    PassKey,
    PolkadotWalletService& polkadot_wallet_service,
    KeyringService& keyring_service,
    mojom::AccountIdPtr sender_account_id,
    std::string chain_id,
    base::span<const uint8_t> extrinsic,
    uint32_t block_num,
    uint32_t mortality_period)
    : polkadot_wallet_service_(polkadot_wallet_service),
      keyring_service_(keyring_service),
      sender_account_id_(std::move(sender_account_id)),
      chain_id_(std::move(chain_id)),
      extrinsic_hex_(base::StrCat({"0x", base::HexEncodeLower(extrinsic)})),
      block_num_{block_num},
      mortality_period_{mortality_period},
      curr_block_num_{block_num} {
  // Use saturating addition here for the sake of simplicity. While it's
  // unlikely, a block produced near the end of the chain can still have an
  // extrinsic within its mortality window even if the true end of the mortality
  // window exceeds numeric limits for a uint32_t.
  max_block_num_ = base::ClampAdd(block_num_, mortality_period_);
}

PolkadotTransactionStatusTask::~PolkadotTransactionStatusTask() = default;

PolkadotSubstrateRpc* PolkadotTransactionStatusTask::GetPolkadotRpc() {
  return polkadot_wallet_service_->GetPolkadotRpc();
}

void PolkadotTransactionStatusTask::HandleError(std::string err_str) {
  std::move(callback_).Run(base::unexpected(std::move(err_str)));
}

void PolkadotTransactionStatusTask::Start(
    GetTransactionStatusCallback callback) {
  if (initiated_) {
    return std::move(callback).Run(
        base::unexpected(WalletInternalErrorMessage()));
  }

  initiated_ = true;
  callback_ = std::move(callback);

  polkadot_wallet_service_->GetChainMetadata(
      chain_id_,
      base::BindOnce(&PolkadotTransactionStatusTask::OnGetChainMetadata,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotTransactionStatusTask::OnGetChainMetadata(
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

void PolkadotTransactionStatusTask::OnGetFinalizedHead(
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
      base::BindOnce(&PolkadotTransactionStatusTask::OnGetFinalizedBlockHeader,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotTransactionStatusTask::OnGetFinalizedBlockHeader(
    std::optional<PolkadotBlockHeader> block_header,
    std::optional<std::string> err_str) {
  if (err_str) {
    return HandleError(std::move(*err_str));
  }

  if (!block_header) {
    return HandleError(WalletInternalErrorMessage());
  }

  finalized_block_num_ = block_header->block_number;
  FetchCurrentBlock();
}

void PolkadotTransactionStatusTask::FetchCurrentBlock() {
  // Fetch the block with number curr_block_num_ from the connected relay/para
  // chain. The RPC API only permits: block_num -> block_hash -> block
  // so we first begin by grabbing the block hash, then we can use that to grab
  // the entire block. We grab the entire block for the extrinsics array. Once
  // we have the block, we can search the extrinsics for the one we're
  // interested in. The index where the extrinsic is located is what we use to
  // probe the block's events for the extrinsic status and the true fee paid by
  // the sender.
  //
  // If curr_block_num_ exceeds the finalized head, the extrinsic hasn't
  // been included yet but may be in the future (kNotFinalized).
  // If curr_block_num_ reaches max_block_num_ (signing block + mortality),
  // the extrinsic was not found (kNotFound). This is because it was dropped by
  // the mempool.
  // Otherwise, begin the hash → block request chain below.

  if (curr_block_num_ > finalized_block_num_) {
    return std::move(callback_).Run(base::ok(
        std::pair(PolkadotTransactionStatus::kNotFinalized, std::nullopt)));
  }

  if (curr_block_num_ >= max_block_num_) {
    return std::move(callback_).Run(base::ok(
        std::pair(PolkadotTransactionStatus::kNotFound, std::nullopt)));
  }

  GetPolkadotRpc()->GetBlockHash(
      chain_id_, curr_block_num_,
      base::BindOnce(&PolkadotTransactionStatusTask::OnGetBlockHashForStatus,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotTransactionStatusTask::OnGetBlockHashForStatus(
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

void PolkadotTransactionStatusTask::OnGetBlockForStatus(
    std::optional<PolkadotBlock> block,
    std::optional<std::string> err_str) {
  if (err_str) {
    return HandleError(std::move(*err_str));
  }

  if (!block) {
    return HandleError(WalletInternalErrorMessage());
  }

  auto idx = FindPosition(block->extrinsics, extrinsic_hex_);
  if (idx.has_value()) {
    extrinsic_idx_ = idx;
    GetPolkadotRpc()->GetEvents(
        chain_id_, block->header.GetHash(),
        base::BindOnce(&PolkadotTransactionStatusTask::OnGetEvents,
                       weak_ptr_factory_.GetWeakPtr()));

    return;
  }

  if (!base::CheckAdd(curr_block_num_, 1).AssignIfValid(&curr_block_num_)) {
    return std::move(callback_).Run(base::ok(
        std::pair(PolkadotTransactionStatus::kNotFound, std::nullopt)));
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&PolkadotTransactionStatusTask::FetchCurrentBlock,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotTransactionStatusTask::OnGetEvents(
    base::expected<std::vector<uint8_t>, std::string> events) {
  if (!events.has_value()) {
    return HandleError(std::move(events.error()));
  }

  auto pubkey = keyring_service_->GetPolkadotPubKey(sender_account_id_);
  CHECK(pubkey);

  std::array<uint8_t, 16> fee_bytes = {};

  bool was_successful = was_extrinsic_successful(
      ::rust::Slice<const uint8_t>(events.value()), *extrinsic_idx_, *pubkey,
      **chain_metadata_, fee_bytes);

  auto fee = base::bit_cast<uint128_t>(fee_bytes);
  if (fee == 0) {
    // If our fee is 0 here, it means that we received an invalid response from
    // the peer. This is a hard guarantee in Polkadot because including an
    // extrinsic in a finalized block incurs a base_fee, which is non-zero.
    return std::move(callback_).Run(base::ok(
        std::pair(PolkadotTransactionStatus::kInvalidResponse, std::nullopt)));
  }

  std::move(callback_).Run(
      base::ok(std::pair(was_successful ? PolkadotTransactionStatus::kSuccess
                                        : PolkadotTransactionStatus::kFailed,
                         fee)));
}

}  // namespace brave_wallet
