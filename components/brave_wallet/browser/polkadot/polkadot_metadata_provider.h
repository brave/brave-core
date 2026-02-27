/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_METADATA_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_METADATA_PROVIDER_H_

#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata_prefs.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

namespace brave_wallet {

class PolkadotMetadataProvider {
 public:
  using GetChainMetadataCallback = base::OnceCallback<void(
      base::expected<PolkadotChainMetadata, std::string>)>;

  PolkadotMetadataProvider(PolkadotChainMetadataPrefs& chain_metadata_prefs,
                           PolkadotSubstrateRpc& polkadot_substrate_rpc);
  ~PolkadotMetadataProvider();

  // Warms up metadata for known Polkadot networks in background.
  void Init();
  void GetChainMetadata(std::string_view chain_id, GetChainMetadataCallback);

 private:
  void OnGetRuntimeVersion(
      std::string chain_id,
      PolkadotChainMetadata saved_metadata,
      GetChainMetadataCallback callback,
      std::optional<PolkadotRuntimeVersion> runtime_version,
      std::optional<std::string> err_str);

  void FetchAndParseChainMetadata(std::string chain_id,
                                  GetChainMetadataCallback callback);

  void OnGetMetadata(std::string chain_id,
                     GetChainMetadataCallback callback,
                     std::optional<std::string> metadata_hex,
                     std::optional<std::string> err_str);

  void SetCachedMetadata(std::string_view chain_id,
                         const PolkadotChainMetadata& metadata);

  void OnInitChainMetadataResult(
      std::string chain_id,
      uint32_t attempt,
      base::expected<PolkadotChainMetadata, std::string> result);
  void RetryInitChainMetadata(std::string chain_id, uint32_t attempt);

  const raw_ref<PolkadotChainMetadataPrefs> chain_metadata_prefs_;
  const raw_ref<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
  absl::flat_hash_map<std::string, PolkadotChainMetadata> metadata_cache_;
  bool init_started_ = false;
  static constexpr uint32_t kMaxInitAttempts = 5;
  static constexpr base::TimeDelta kInitRetryDelay = base::Seconds(5);
  base::WeakPtrFactory<PolkadotMetadataProvider> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_METADATA_PROVIDER_H_
