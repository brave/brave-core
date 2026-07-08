// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_COMPLETE_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_COMPLETE_TRANSACTION_TASK_H_

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_storage.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

namespace brave_wallet {

class KeyringService;
class ZCashWalletService;

// Completes transaction by signing transparent inputs and generating orchard
// part(if needed).
class ZCashCompleteTransactionTask {
 public:
  using ZCashCompleteTransactionTaskCallback =
      base::OnceCallback<void(base::expected<ZCashTransaction, std::string>)>;
  ZCashCompleteTransactionTask(base::PassKey<ZCashWalletService> pass_key,
                               ZCashWalletService& zcash_wallet_service,
                               ZCashActionContext context,
                               KeyringService& keyring_service,
                               const ZCashTransaction& transaction);
  ~ZCashCompleteTransactionTask();

  void Start(ZCashCompleteTransactionTaskCallback callback);

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void GetLightdInfo();
  void OnGetLightdInfo(
      base::expected<zcash::mojom::LightdInfoPtr, std::string> result);

  void GetLatestBlock();
  void OnGetLatestBlockHeight(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);

  void CalculateWitness();
  void OnWitnessCalculateResult(
      base::expected<std::vector<OrchardInput>, OrchardStorage::Error> result);

  void GetTreeState();
  void OnGetTreeState(
      base::expected<zcash::mojom::TreeStatePtr, std::string> result);

  void SignOrchardPart();
  void OnSignOrchardPartComplete(
      std::unique_ptr<OrchardBundleManager> orchard_bundle_manager);

  void CalculateWitnessV6(OrchardPool pool);
  void OnWitnessCalculateResultV6(
      OrchardPool pool,
      base::expected<std::vector<OrchardInput>, OrchardStorage::Error> result);
  void GetTreeStateV6(OrchardPool pool);
  void OnGetTreeStateV6(
      OrchardPool pool,
      base::expected<zcash::mojom::TreeStatePtr, std::string> result);
  void BuildOrchardBundleV6(OrchardPool pool);
  void SignOrchardBundlesV6();
  void OnSignOrchardBundleCompleteV6(
      OrchardPool pool,
      std::unique_ptr<OrchardBundleManager> orchard_bundle_manager);
  ZCashTransaction::ShieldedPool& PoolRefV6(OrchardPool pool);

  void SignTransparentPart();

  raw_ref<ZCashWalletService> zcash_wallet_service_;
  ZCashActionContext context_;
  raw_ref<KeyringService> keyring_service_;
  ZCashTransaction transaction_;
  ZCashCompleteTransactionTaskCallback callback_;

  std::optional<std::string> error_;
  std::optional<uint32_t> consensus_branch_id_;
  std::optional<uint32_t> chain_tip_height_;

  std::optional<std::vector<OrchardInput>> witness_inputs_;
  std::optional<zcash::mojom::TreeStatePtr> anchor_tree_state_;

  struct V6PoolSigningState {
    V6PoolSigningState();
    ~V6PoolSigningState();
    V6PoolSigningState(V6PoolSigningState&&);
    V6PoolSigningState& operator=(V6PoolSigningState&&);
    bool witness_done = false;
    std::optional<zcash::mojom::TreeStatePtr> tree_state;
    std::unique_ptr<OrchardBundleManager> bundle_manager;
  };
  std::map<OrchardPool, V6PoolSigningState> v6_signing_state_;
  std::optional<std::array<uint8_t, kZCashDigestSize>> v6_sighash_;

  base::WeakPtrFactory<ZCashCompleteTransactionTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_COMPLETE_TRANSACTION_TASK_H_
