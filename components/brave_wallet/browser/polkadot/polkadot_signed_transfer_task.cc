/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_signed_transfer_task.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

namespace brave_wallet {

PolkadotSignedTransferTask::PolkadotSignedTransferTask(
    PolkadotWalletService& polkadot_wallet_service,
    KeyringService& keyring_service,
    mojom::AccountIdPtr sender_account_id,
    std::string chain_id,
    uint128_t send_amount,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> sender,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient)
    : polkadot_wallet_service_(polkadot_wallet_service),
      keyring_service_(keyring_service),
      sender_account_id_(std::move(sender_account_id)),
      chain_id_(std::move(chain_id)),
      send_amount_{send_amount} {
  base::span(sender_).copy_from_nonoverlapping(sender);
  base::span(recipient_).copy_from_nonoverlapping(recipient);
}

PolkadotSignedTransferTask::~PolkadotSignedTransferTask() = default;

bool PolkadotSignedTransferTask::IsReadyToSign() const {
  return !account_info_.is_null() && signing_header_ && genesis_hash_ &&
         signing_block_hash_ && runtime_version_ && chain_metadata_;
}

void PolkadotSignedTransferTask::StopWithError(std::string error_string) {
  weak_ptr_factory_.InvalidateWeakPtrs();
  std::move(callback_).Run(base::unexpected(std::move(error_string)));
}

PolkadotSubstrateRpc* PolkadotSignedTransferTask::GetPolkadotRpc() {
  return polkadot_wallet_service_->GetPolkadotRpc();
}

void PolkadotSignedTransferTask::Start(
    GenerateSignedTransferExtrinsicCallback callback) {
  callback_ = std::move(callback);

  GetNonce();
  GetSigningHeader();
  GetGenesisHash();
  polkadot_wallet_service_->GetChainMetadata(
      chain_id_,
      base::BindOnce(&PolkadotSignedTransferTask::OnGetMetadataForSigning,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotSignedTransferTask::GetNonce() {
  GetPolkadotRpc()->GetAccountBalance(
      chain_id_, sender_,
      base::BindOnce(&PolkadotSignedTransferTask::OnGetAccountNonce,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotSignedTransferTask::GetSigningHeader() {
  GetPolkadotRpc()->GetBlockHeader(
      chain_id_, std::nullopt,
      base::BindOnce(&PolkadotSignedTransferTask::OnGetChainHeader,
                     weak_ptr_factory_.GetWeakPtr()));

  GetPolkadotRpc()->GetFinalizedHead(
      chain_id_, base::BindOnce(&PolkadotSignedTransferTask::OnGetFinalizedHead,
                                weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotSignedTransferTask::GetGenesisHash() {
  GetPolkadotRpc()->GetBlockHash(
      chain_id_, 0,
      base::BindOnce(&PolkadotSignedTransferTask::OnGetGenesisHash,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotSignedTransferTask::OnGetMetadataForSigning(
    base::expected<PolkadotChainMetadata, std::string> chain_metadata) {
  if (!chain_metadata.has_value()) {
    return StopWithError(chain_metadata.error());
  }

  chain_metadata_ = std::move(chain_metadata.value());
  MaybeFinalizeSignTransaction();
}

void PolkadotSignedTransferTask::OnGetAccountNonce(
    mojom::PolkadotAccountInfoPtr account_info,
    const std::optional<std::string>& error_string) {
  if (error_string) {
    return StopWithError(*error_string);
  }

  CHECK(account_info);
  account_info_ = std::move(account_info);

  MaybeFinalizeSignTransaction();
}

void PolkadotSignedTransferTask::OnGetChainHeader(
    std::optional<PolkadotBlockHeader> header,
    std::optional<std::string> error_string) {
  if (error_string) {
    return StopWithError(*error_string);
  }

  // Current behavior of the RPC layer seems to be returning an error when
  // there's no parent hash, so we always fetch it in our case vs polkadot-js
  // which seems to assume very young chains where the parent hash doesn't
  // necessarily exist.
  CHECK(header);
  GetPolkadotRpc()->GetBlockHeader(
      chain_id_, header->parent_hash,
      base::BindOnce(&PolkadotSignedTransferTask::OnGetParentHeader,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotSignedTransferTask::OnGetParentHeader(
    std::optional<PolkadotBlockHeader> header,
    std::optional<std::string> error_string) {
  if (error_string) {
    return StopWithError(*error_string);
  }

  CHECK(header);
  chain_header_ = header;
  UpdateSigningHeader();
}

void PolkadotSignedTransferTask::OnGetFinalizedHead(
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> finalized_hash,
    std::optional<std::string> error_string) {
  if (error_string) {
    return StopWithError(*error_string);
  }

  CHECK(finalized_hash);
  GetPolkadotRpc()->GetBlockHeader(
      chain_id_, *finalized_hash,
      base::BindOnce(&PolkadotSignedTransferTask::OnGetFinalizedBlockHeader,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotSignedTransferTask::OnGetFinalizedBlockHeader(
    std::optional<PolkadotBlockHeader> header,
    std::optional<std::string> error_string) {
  if (error_string) {
    return StopWithError(*error_string);
  }

  CHECK(header);
  finalized_header_ = header;
  UpdateSigningHeader();
}

void PolkadotSignedTransferTask::UpdateSigningHeader() {
  if (!chain_header_ || !finalized_header_) {
    return;
  }

  if (signing_header_) {
    return;
  }

  auto current = chain_header_->block_number;
  auto finalized = finalized_header_->block_number;

  constexpr uint32_t kMaxFinalityLag = 5;
  if (current - finalized > kMaxFinalityLag) {
    signing_header_ = chain_header_;
  } else {
    signing_header_ = finalized_header_;
  }

  GetPolkadotRpc()->GetBlockHash(
      chain_id_, signing_header_->block_number,
      base::BindOnce(&PolkadotSignedTransferTask::OnGetSigningBlockHash,
                     weak_ptr_factory_.GetWeakPtr()));

  GetPolkadotRpc()->GetRuntimeVersion(
      chain_id_, signing_header_->parent_hash,
      base::BindOnce(&PolkadotSignedTransferTask::OnGetRuntimeVersion,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PolkadotSignedTransferTask::OnGetGenesisHash(
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> genesis_hash,
    std::optional<std::string> error_string) {
  if (error_string) {
    return StopWithError(*error_string);
  }

  CHECK(genesis_hash);
  genesis_hash_ = genesis_hash;
  MaybeFinalizeSignTransaction();
}

void PolkadotSignedTransferTask::OnGetSigningBlockHash(
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
    std::optional<std::string> error_string) {
  if (error_string) {
    return StopWithError(*error_string);
  }

  CHECK(block_hash);
  signing_block_hash_ = block_hash;
  MaybeFinalizeSignTransaction();
}

void PolkadotSignedTransferTask::OnGetRuntimeVersion(
    std::optional<PolkadotRuntimeVersion> runtime_version,
    std::optional<std::string> error_string) {
  if (error_string) {
    return StopWithError(*error_string);
  }

  CHECK(runtime_version);

  runtime_version_ = runtime_version;
  MaybeFinalizeSignTransaction();
}

void PolkadotSignedTransferTask::MaybeFinalizeSignTransaction() {
  if (!IsReadyToSign()) {
    return;
  }

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount_));

  auto signature_payload = generate_extrinsic_signature_payload(
      **chain_metadata_, account_info_->nonce, send_amount_bytes, recipient_,
      runtime_version_->spec_version, runtime_version_->transaction_version,
      signing_header_->block_number, *genesis_hash_, *signing_block_hash_);

  auto signature = keyring_service_->SignMessageByPolkadotKeyring(
      sender_account_id_, signature_payload);

  CHECK(signature);

  auto signed_extrinsic_bytes = make_signed_extrinsic(
      **chain_metadata_,
      *keyring_service_->GetPolkadotPubKey(sender_account_id_), recipient_,
      send_amount_bytes, *signature, signing_header_->block_number,
      account_info_->nonce);

  std::move(callback_).Run(
      base::ok(base::HexEncodeLower(signed_extrinsic_bytes)));
}

}  // namespace brave_wallet
