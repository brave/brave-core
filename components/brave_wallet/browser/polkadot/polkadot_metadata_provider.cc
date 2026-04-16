/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_metadata_provider.h"

#include <vector>

#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

namespace {

base::expected<PolkadotChainMetadata, std::string> ParseChainMetadataFromHex(
    base::expected<std::vector<uint8_t>, std::string> metadata_bytes_or_error) {
  if (!metadata_bytes_or_error.has_value()) {
    return base::unexpected(metadata_bytes_or_error.error());
  }

  auto chain_metadata =
      PolkadotChainMetadata::FromBytes(metadata_bytes_or_error.value());
  if (!chain_metadata) {
    return base::unexpected("Failed to parse runtime metadata.");
  }

  return base::ok(std::move(*chain_metadata));
}

}  // namespace

PolkadotMetadataProvider::PolkadotMetadataProvider(
    PolkadotChainMetadataPrefs& chain_metadata_prefs,
    PolkadotSubstrateRpc& polkadot_substrate_rpc)
    : chain_metadata_prefs_(chain_metadata_prefs),
      polkadot_substrate_rpc_(polkadot_substrate_rpc) {}

PolkadotMetadataProvider::~PolkadotMetadataProvider() = default;

void PolkadotMetadataProvider::Init() {
  if (init_started_) {
    return;
  }
  init_started_ = true;

  RetryInitChainMetadata(mojom::kPolkadotMainnet, /*attempt=*/1);
  RetryInitChainMetadata(mojom::kPolkadotTestnet, /*attempt=*/1);
}

void PolkadotMetadataProvider::GetChainMetadata(
    std::string_view chain_id,
    GetChainMetadataCallback callback) {
  CHECK(IsPolkadotNetwork(chain_id));

  if (const auto* cached_metadata =
          base::FindOrNull(metadata_cache_, chain_id)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), base::ok(*cached_metadata)));
    return;
  }

  auto saved_metadata = chain_metadata_prefs_->GetChainMetadata(chain_id);
  if (!saved_metadata) {
    FetchAndParseChainMetadata(std::string(chain_id), std::move(callback));
    return;
  }

  polkadot_substrate_rpc_->GetRuntimeVersion(
      chain_id, std::nullopt,
      base::BindOnce(&PolkadotMetadataProvider::OnGetRuntimeVersion,
                     weak_ptr_factory_.GetWeakPtr(), std::string(chain_id),
                     std::move(*saved_metadata), std::move(callback)));
}

void PolkadotMetadataProvider::OnGetRuntimeVersion(
    std::string chain_id,
    PolkadotChainMetadata saved_metadata,
    GetChainMetadataCallback callback,
    std::optional<PolkadotRuntimeVersion> runtime_version,
    std::optional<std::string> err_str) {
  if (err_str) {
    std::move(callback).Run(base::unexpected(*err_str));
    return;
  }

  if (!runtime_version) {
    std::move(callback).Run(base::unexpected(WalletParsingErrorMessage()));
    return;
  }

  if (saved_metadata.GetSpecVersion() == runtime_version->spec_version) {
    SetCachedMetadata(chain_id, saved_metadata);
    std::move(callback).Run(base::ok(std::move(saved_metadata)));
    return;
  }

  FetchAndParseChainMetadata(std::move(chain_id), std::move(callback));
}

void PolkadotMetadataProvider::FetchAndParseChainMetadata(
    std::string chain_id,
    GetChainMetadataCallback callback) {
  polkadot_substrate_rpc_->GetMetadata(
      chain_id, base::BindOnce(&PolkadotMetadataProvider::OnGetMetadata,
                               weak_ptr_factory_.GetWeakPtr(), chain_id,
                               std::move(callback)));
}

void PolkadotMetadataProvider::OnGetMetadata(
    std::string chain_id,
    GetChainMetadataCallback callback,
    base::expected<std::vector<uint8_t>, std::string> metadata_bytes_or_error) {
  auto parsed = ParseChainMetadataFromHex(std::move(metadata_bytes_or_error));
  if (!parsed.has_value()) {
    std::move(callback).Run(base::unexpected(parsed.error()));
    return;
  }

  if (!chain_metadata_prefs_->SetChainMetadata(chain_id, *parsed)) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }
  SetCachedMetadata(chain_id, *parsed);
  std::move(callback).Run(base::ok(std::move(*parsed)));
}

void PolkadotMetadataProvider::SetCachedMetadata(
    std::string_view chain_id,
    const PolkadotChainMetadata& metadata) {
  PolkadotChainMetadata metadata_copy(metadata);
  metadata_cache_.insert_or_assign(std::string(chain_id),
                                   std::move(metadata_copy));
}

void PolkadotMetadataProvider::OnInitChainMetadataResult(
    std::string chain_id,
    uint32_t attempt,
    base::expected<PolkadotChainMetadata, std::string> result) {
  if (!result.has_value()) {
    if (attempt < kMaxInitAttempts) {
      base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&PolkadotMetadataProvider::RetryInitChainMetadata,
                         weak_ptr_factory_.GetWeakPtr(), std::move(chain_id),
                         attempt + 1),
          kInitRetryDelay);
    }
    return;
  }
}

void PolkadotMetadataProvider::RetryInitChainMetadata(std::string chain_id,
                                                      uint32_t attempt) {
  GetChainMetadata(
      chain_id,
      base::BindOnce(&PolkadotMetadataProvider::OnInitChainMetadataResult,
                     weak_ptr_factory_.GetWeakPtr(), chain_id, attempt));
}

}  // namespace brave_wallet
