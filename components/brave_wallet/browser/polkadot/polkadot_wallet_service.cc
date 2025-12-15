/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include "base/strings/string_number_conversions.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.rs.h"  // IWYU pragma: export
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {
namespace {

base::expected<PolkadotChainMetadata, std::string> ParseChainMetadataReponse(
    base::optional_ref<const std::string> chain_name,
    base::optional_ref<const std::string> err_str) {
  if (err_str) {
    return base::unexpected(*err_str);
  }

  CHECK(chain_name);
  auto chain_metadata = PolkadotChainMetadata::FromChainName(*chain_name);
  if (!chain_metadata) {
    return base::unexpected(
        "Failed to parse metadata for the provided chain spec.");
  }

  return base::ok(std::move(*chain_metadata));
}

}  // namespace

PolkadotWalletService::PolkadotWalletService(
    KeyringService& keyring_service,
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      polkadot_substrate_rpc_(network_manager, std::move(url_loader_factory)) {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

PolkadotWalletService::~PolkadotWalletService() = default;

void PolkadotWalletService::GetChainMetadata(
    std::string_view chain_id,
    GetChainMetadataCallback callback) {
  CHECK(IsPolkadotNetwork(chain_id));

  if (chain_id == mojom::kPolkadotTestnet) {
    if (testnet_chain_metadata_) {
      return std::move(callback).Run(*testnet_chain_metadata_);
    } else {
      // Testnet chain metadata isn't ready yet, defer execution of the
      // callback.
      testnet_chain_metadata_callbacks_.push_back(std::move(callback));
    }
  }

  if (chain_id == mojom::kPolkadotMainnet) {
    if (mainnet_chain_metadata_) {
      return std::move(callback).Run(*mainnet_chain_metadata_);
    } else {
      // Mainnet chain metadata isn't ready yet, defer execution of the
      // callback.
      mainnet_chain_metadata_callbacks_.push_back(std::move(callback));
    }
  }
}

void PolkadotWalletService::Bind(
    mojo::PendingReceiver<mojom::PolkadotWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void PolkadotWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void PolkadotWalletService::GetNetworkName(mojom::AccountIdPtr account_id,
                                           GetNetworkNameCallback callback) {
  std::string chain_id = GetNetworkForPolkadotAccount(account_id);
  polkadot_substrate_rpc_.GetChainName(std::move(chain_id),
                                       std::move(callback));
}

void PolkadotWalletService::GetAccountBalance(
    mojom::AccountIdPtr account_id,
    const std::string& chain_id,
    GetAccountBalanceCallback callback) {
  auto pubkey = keyring_service_->GetPolkadotPubKey(account_id);
  if (!pubkey) {
    return std::move(callback).Run(nullptr, WalletInternalErrorMessage());
  }

  polkadot_substrate_rpc_.GetAccountBalance(chain_id, *pubkey,
                                            std::move(callback));
}

void PolkadotWalletService::Unlocked() {
  InitializeChainMetadata();
}

void PolkadotWalletService::InitializeChainMetadata() {
  if (testnet_chain_metadata_ && mainnet_chain_metadata_) {
    return;
  }

  polkadot_substrate_rpc_.GetChainName(
      mojom::kPolkadotTestnet,
      base::BindOnce(&PolkadotWalletService::OnInitializeChainMetadata,
                     weak_ptr_factory_.GetWeakPtr(), mojom::kPolkadotTestnet));

  polkadot_substrate_rpc_.GetChainName(
      mojom::kPolkadotMainnet,
      base::BindOnce(&PolkadotWalletService::OnInitializeChainMetadata,
                     weak_ptr_factory_.GetWeakPtr(), mojom::kPolkadotMainnet));
}

void PolkadotWalletService::OnInitializeChainMetadata(
    std::string_view chain_id,
    const std::optional<std::string>& chain_name,
    const std::optional<std::string>& err_str) {
  CHECK(IsPolkadotNetwork(chain_id));

  if (chain_id == mojom::kPolkadotTestnet) {
    testnet_chain_metadata_ = ParseChainMetadataReponse(chain_name, err_str);
    for (auto& callback : testnet_chain_metadata_callbacks_) {
      std::move(callback).Run(*testnet_chain_metadata_);
    }
    testnet_chain_metadata_callbacks_.clear();
  }

  if (chain_id == mojom::kPolkadotMainnet) {
    mainnet_chain_metadata_ = ParseChainMetadataReponse(chain_name, err_str);
    for (auto& callback : mainnet_chain_metadata_callbacks_) {
      std::move(callback).Run(*mainnet_chain_metadata_);
    }
    mainnet_chain_metadata_callbacks_.clear();
  }
}

PolkadotWalletService::PolkadotSignTransaction::PolkadotSignTransaction() =
    default;
PolkadotWalletService::PolkadotSignTransaction::~PolkadotSignTransaction() =
    default;

bool PolkadotWalletService::PolkadotSignTransaction::is_done() const {
  return !account_info.is_null() && signing_header && genesis_hash &&
         signing_block_hash && runtime_version && chain_metadata;
}

void PolkadotWalletService::UpdateSigningHeader(
    scoped_refptr<PolkadotSignTransaction> transaction_state) {
  if (!transaction_state->chain_header ||
      !transaction_state->finalized_header) {
    return;
  }

  if (transaction_state->signing_header) {
    return;
  }

  auto current = transaction_state->chain_header->block_number;
  auto finalized = transaction_state->finalized_header->block_number;

  constexpr uint32_t kMaxFinalityLag = 5;
  if (current - finalized > kMaxFinalityLag) {
    transaction_state->signing_header = transaction_state->chain_header;
  } else {
    transaction_state->signing_header = transaction_state->finalized_header;
  }

  polkadot_substrate_rpc_.GetBlockHash(
      transaction_state->chain_id,
      transaction_state->signing_header->block_number,
      base::BindOnce(&PolkadotWalletService::OnGetSigningBlockHash,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));

  polkadot_substrate_rpc_.GetRuntimeVersion(
      transaction_state->chain_id,
      transaction_state->signing_header->parent_hash,
      base::BindOnce(&PolkadotWalletService::OnGetRuntimeVersion,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));
}

void PolkadotWalletService::GenerateSignedTransferExtrinsic(
    std::string_view chain_id,
    const mojom::AccountIdPtr& account_id,
    uint128_t send_amount,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    GenerateSignedTransferExtrinsicCallback callback) {
  auto pubkey = keyring_service_->GetPolkadotPubKey(account_id);
  if (!pubkey) {
    return std::move(callback).Run(
        base::unexpected(WalletInternalErrorMessage()));
  }

  scoped_refptr<PolkadotSignTransaction> transaction_state =
      base::MakeRefCounted<PolkadotSignTransaction>();

  transaction_state->sender_account_id = account_id->Clone();
  transaction_state->callback = std::move(callback);
  transaction_state->chain_id = chain_id;
  transaction_state->send_amount = send_amount;
  base::span(transaction_state->recipient).copy_from_nonoverlapping(recipient);

  GetChainMetadata(
      chain_id,
      base::BindOnce(&PolkadotWalletService::OnGetMetadataForSigning,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));

  polkadot_substrate_rpc_.GetAccountBalance(
      chain_id, *pubkey,
      base::BindOnce(&PolkadotWalletService::OnGetAccount,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));

  GetSigningHeader(transaction_state);

  polkadot_substrate_rpc_.GetBlockHash(
      chain_id, 0,
      base::BindOnce(&PolkadotWalletService::OnGetGenesisHash,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));
}

void PolkadotWalletService::SignAndSendTransaction(
    std::string_view chain_id,
    const mojom::AccountIdPtr& account_id,
    uint128_t send_amount,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    SignAndSendTransactionCallback callback) {
  GenerateSignedTransferExtrinsic(
      chain_id, account_id, send_amount, recipient,
      base::BindOnce(&PolkadotWalletService::OnGenerateSignedTransfer,
                     weak_ptr_factory_.GetWeakPtr(), std::string(chain_id),
                     std::move(callback)));
}

void PolkadotWalletService::GetSigningHeader(
    scoped_refptr<PolkadotSignTransaction> transaction_state) {
  polkadot_substrate_rpc_.GetBlockHeader(
      transaction_state->chain_id, std::nullopt,
      base::BindOnce(&PolkadotWalletService::OnGetChainHeader,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));

  polkadot_substrate_rpc_.GetFinalizedHead(
      transaction_state->chain_id,
      base::BindOnce(&PolkadotWalletService::OnGetFinalizedHead,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));
}

void PolkadotWalletService::OnGetMetadataForSigning(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    const base::expected<PolkadotChainMetadata, std::string>& chain_metadata) {
  CHECK(chain_metadata.has_value());

  transaction_state->chain_metadata = &chain_metadata.value();
  OnFinalizeSignTransaction(transaction_state);
}

void PolkadotWalletService::OnGetAccount(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    mojom::PolkadotAccountInfoPtr account_info,
    const std::optional<std::string>&) {
  CHECK(account_info);
  transaction_state->account_info = std::move(account_info);

  OnFinalizeSignTransaction(transaction_state);
}

void PolkadotWalletService::OnGetChainHeader(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    std::optional<PolkadotBlockHeader> header,
    std::optional<std::string>) {
  CHECK(header);

  polkadot_substrate_rpc_.GetBlockHeader(
      transaction_state->chain_id, header->parent_hash,
      base::BindOnce(&PolkadotWalletService::OnGetParentHeader,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));
}

void PolkadotWalletService::OnGetParentHeader(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    std::optional<PolkadotBlockHeader> header,
    std::optional<std::string>) {
  CHECK(header);

  transaction_state->chain_header = header;
  UpdateSigningHeader(transaction_state);
}

void PolkadotWalletService::OnGetFinalizedHead(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> finalized_hash,
    std::optional<std::string>) {
  CHECK(finalized_hash);

  polkadot_substrate_rpc_.GetBlockHeader(
      transaction_state->chain_id, *finalized_hash,
      base::BindOnce(&PolkadotWalletService::OnGetFinalizedBlockHeader,
                     weak_ptr_factory_.GetWeakPtr(), transaction_state));
}

void PolkadotWalletService::OnGetFinalizedBlockHeader(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    std::optional<PolkadotBlockHeader> header,
    std::optional<std::string>) {
  CHECK(header);
  transaction_state->finalized_header = header;
  UpdateSigningHeader(transaction_state);
}

void PolkadotWalletService::OnGetGenesisHash(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> genesis_hash,
    std::optional<std::string>) {
  CHECK(genesis_hash);
  transaction_state->genesis_hash = genesis_hash;
  OnFinalizeSignTransaction(transaction_state);
}

void PolkadotWalletService::OnGetSigningBlockHash(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
    std::optional<std::string>) {
  CHECK(block_hash);
  transaction_state->signing_block_hash = block_hash;
  OnFinalizeSignTransaction(transaction_state);
}

void PolkadotWalletService::OnGetRuntimeVersion(
    scoped_refptr<PolkadotSignTransaction> transaction_state,
    std::optional<PolkadotRuntimeVersion> runtime_version,
    std::optional<std::string>) {
  CHECK(runtime_version);

  transaction_state->runtime_version = runtime_version;
  OnFinalizeSignTransaction(transaction_state);
}

void PolkadotWalletService::OnFinalizeSignTransaction(
    scoped_refptr<PolkadotSignTransaction> transaction_state) {
  if (!transaction_state->is_done()) {
    return;
  }

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(transaction_state->send_amount));

  auto signature_payload = generate_extrinsic_signature_payload(
      **transaction_state->chain_metadata,
      transaction_state->account_info->nonce, send_amount_bytes,
      transaction_state->recipient,
      transaction_state->runtime_version->spec_version,
      transaction_state->runtime_version->transaction_version,
      transaction_state->signing_header->block_number,
      *transaction_state->genesis_hash, *transaction_state->signing_block_hash);

  auto signature = keyring_service_->SignMessageByPolkadotKeyring(
      transaction_state->sender_account_id, signature_payload);

  CHECK(signature);

  auto signed_extrinsic_bytes = make_signed_extrinsic(
      **transaction_state->chain_metadata,
      *keyring_service_->GetPolkadotPubKey(
          transaction_state->sender_account_id),
      transaction_state->recipient, send_amount_bytes, *signature,
      transaction_state->signing_header->block_number,
      transaction_state->account_info->nonce);

  std::move(transaction_state->callback)
      .Run(base::ok(base::HexEncodeLower(signed_extrinsic_bytes)));
}

void PolkadotWalletService::OnGenerateSignedTransfer(
    std::string chain_id,
    SignAndSendTransactionCallback callback,
    base::expected<std::string, std::string> signed_extrinsic) {
  if (!signed_extrinsic.has_value()) {
    return std::move(callback).Run(std::move(signed_extrinsic));
  }

  polkadot_substrate_rpc_.SubmitExtrinsic(
      chain_id, signed_extrinsic.value(),
      base::BindOnce(&PolkadotWalletService::OnSubmitSignedExtrinsic,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotWalletService::OnSubmitSignedExtrinsic(
    SignAndSendTransactionCallback callback,
    std::optional<std::string> transaction_hash,
    std::optional<std::string> error_str) {
  CHECK(transaction_hash);

  if (error_str) {
    return std::move(callback).Run(base::unexpected(*error_str));
  }

  std::move(callback).Run(base::ok(*transaction_hash));
}

}  // namespace brave_wallet
