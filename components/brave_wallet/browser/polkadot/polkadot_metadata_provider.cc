/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_metadata_provider.h"

#include <array>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace {

base::expected<PolkadotChainMetadata, std::string> ParseChainMetadataFromHex(
    const std::optional<std::string>& metadata_hex,
    const std::optional<std::string>& err_str) {
  if (err_str) {
    return base::unexpected(*err_str);
  }

  if (!metadata_hex) {
    return base::unexpected(WalletParsingErrorMessage());
  }

  auto chain_metadata = PolkadotChainMetadata::FromMetadataHex(*metadata_hex);
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
      polkadot_substrate_rpc_(polkadot_substrate_rpc) {
  LOG(ERROR) << "XXXZZZ PolkadotMetadataProvider constructed";
}

PolkadotMetadataProvider::~PolkadotMetadataProvider() = default;

void PolkadotMetadataProvider::Init() {
  if (init_started_) {
    LOG(ERROR) << "XXXZZZ metadata init skipped reason=already_started";
    return;
  }
  init_started_ = true;
  LOG(ERROR) << "XXXZZZ metadata init start";

  RetryInitChainMetadata(mojom::kPolkadotMainnet, /*attempt=*/1);
  RetryInitChainMetadata(mojom::kPolkadotTestnet, /*attempt=*/1);

}

void PolkadotMetadataProvider::GetChainMetadata(
    std::string_view chain_id,
    GetChainMetadataCallback callback) {
  LOG(ERROR) << "XXXZZZ GetChainMetadata start chain_id=" << chain_id;

  CHECK(IsPolkadotNetwork(chain_id));

  auto cache_it = metadata_cache_.find(chain_id);
  if (cache_it != metadata_cache_.end()) {
    LOG(ERROR) << "XXXZZZ parsed chain metadata source=cache cache_layer=memory"
               << " chain_id=" << chain_id
               << " balances_pallet_index="
               << static_cast<int>(cache_it->second.GetBalancesPalletIndex())
               << " transfer_allow_death_call_index="
               << static_cast<int>(
                      cache_it->second.GetTransferAllowDeathCallIndex())
               << " ss58_prefix=" << cache_it->second.GetSs58Prefix()
               << " spec_version=" << cache_it->second.GetSpecVersion();
    std::move(callback).Run(base::ok(cache_it->second));
    return;
  }

  auto saved_metadata = chain_metadata_prefs_->GetChainMetadata(chain_id);
  if (!saved_metadata) {
    LOG(ERROR) << "XXXZZZ metadata cache miss in prefs chain_id=" << chain_id
               << " source=network";
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
    LOG(ERROR) << "XXXZZZ runtime version fetch failed chain_id=" << chain_id
               << " err=" << *err_str;
    std::move(callback).Run(base::unexpected(*err_str));
    return;
  }

  if (!runtime_version) {
    LOG(ERROR) << "XXXZZZ runtime version response missing chain_id=" << chain_id;
    std::move(callback).Run(base::unexpected(WalletParsingErrorMessage()));
    return;
  }

  if (saved_metadata.GetSpecVersion() == runtime_version->spec_version) {
    LOG(ERROR) << "XXXZZZ parsed chain metadata source=cache cache_layer=prefs"
               << " chain_id=" << chain_id
               << " balances_pallet_index="
               << static_cast<int>(saved_metadata.GetBalancesPalletIndex())
               << " transfer_allow_death_call_index="
               << static_cast<int>(
                      saved_metadata.GetTransferAllowDeathCallIndex())
               << " ss58_prefix=" << saved_metadata.GetSs58Prefix()
               << " spec_version=" << saved_metadata.GetSpecVersion();
    SetCachedMetadata(chain_id, saved_metadata);
    std::move(callback).Run(base::ok(std::move(saved_metadata)));
    return;
  }

  LOG(ERROR) << "XXXZZZ runtime version mismatch chain_id=" << chain_id
             << " saved_spec_version=" << saved_metadata.GetSpecVersion()
             << " runtime_spec_version=" << runtime_version->spec_version
             << " source=network";
  FetchAndParseChainMetadata(std::move(chain_id), std::move(callback));
}

void PolkadotMetadataProvider::FetchAndParseChainMetadata(
    std::string chain_id,
    GetChainMetadataCallback callback) {
  LOG(ERROR) << "XXXZZZ fetching metadata from network chain_id=" << chain_id;
  const std::string chain_id_for_rpc = chain_id;
  polkadot_substrate_rpc_->GetMetadata(
      chain_id_for_rpc,
      base::BindOnce(&PolkadotMetadataProvider::OnGetMetadata,
                     weak_ptr_factory_.GetWeakPtr(), std::move(chain_id),
                     std::move(callback)));
}

void PolkadotMetadataProvider::OnGetMetadata(
    std::string chain_id,
    GetChainMetadataCallback callback,
    std::optional<std::string> metadata_hex,
    std::optional<std::string> err_str) {
  LOG(ERROR) << "XXXZZZ metadata rpc response chain_id=" << chain_id
             << " has_err=" << (err_str.has_value() ? "true" : "false")
             << " has_result=" << (metadata_hex.has_value() ? "true" : "false")
             << " result_len=" << (metadata_hex ? metadata_hex->size() : 0u)
             << " err=" << (err_str ? *err_str : "");

  auto parsed = ParseChainMetadataFromHex(metadata_hex, err_str);
  if (!parsed.has_value()) {
    const bool parser_failed =
        !err_str.has_value() && metadata_hex.has_value() && !metadata_hex->empty();
    LOG(ERROR) << "XXXZZZ metadata parse failed chain_id=" << chain_id
               << " err=" << parsed.error()
               << " parser_failed=" << (parser_failed ? "true" : "false");
    std::move(callback).Run(base::unexpected(parsed.error()));
    return;
  }

  LOG(ERROR) << "XXXZZZ parsed chain metadata source=network"
             << " chain_id=" << chain_id << " balances_pallet_index="
             << static_cast<int>(parsed->GetBalancesPalletIndex())
             << " transfer_allow_death_call_index="
             << static_cast<int>(parsed->GetTransferAllowDeathCallIndex())
             << " ss58_prefix=" << parsed->GetSs58Prefix()
             << " spec_version=" << parsed->GetSpecVersion();

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
    LOG(ERROR) << "XXXZZZ metadata init failed chain_id=" << chain_id
               << " err=" << result.error() << " attempt=" << attempt;
    if (attempt < kMaxInitAttempts) {
      LOG(ERROR) << "XXXZZZ metadata init retry scheduled chain_id=" << chain_id
                 << " next_attempt=" << (attempt + 1)
                 << " delay_ms=" << kInitRetryDelay.InMilliseconds();
      base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&PolkadotMetadataProvider::RetryInitChainMetadata,
                         weak_ptr_factory_.GetWeakPtr(), std::move(chain_id),
                         attempt + 1),
          kInitRetryDelay);
    }
    return;
  }

  LOG(ERROR) << "XXXZZZ metadata init ready chain_id=" << chain_id
             << " attempt=" << attempt;
}

void PolkadotMetadataProvider::RetryInitChainMetadata(std::string chain_id,
                                                      uint32_t attempt) {
  LOG(ERROR) << "XXXZZZ metadata init retry start chain_id=" << chain_id;
  GetChainMetadata(
      chain_id,
      base::BindOnce(&PolkadotMetadataProvider::OnInitChainMetadataResult,
                     weak_ptr_factory_.GetWeakPtr(), chain_id,
                     attempt));
}

}  // namespace brave_wallet
