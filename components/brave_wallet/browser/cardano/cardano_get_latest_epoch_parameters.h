/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_GET_LATEST_EPOCH_PARAMETERS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_GET_LATEST_EPOCH_PARAMETERS_TASK_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {
class CardanoWalletService;

class GetLatestEpochParametersTask {
 public:
  using Callback = base::OnceCallback<void(
      base::expected<cardano_rpc::EpochParameters, std::string>)>;

  GetLatestEpochParametersTask(CardanoWalletService& cardano_wallet_service,
                               const std::string& chain_id,
                               std::vector<mojom::CardanoAddressPtr> addresses);
  virtual ~GetLatestEpochParametersTask();

  void set_callback(Callback callback) { callback_ = std::move(callback); }

  void ScheduleWorkOnTask();

 private:
  void WorkOnTask();
  void MaybeSendRequests();
  void OnGetUtxoList(
      mojom::CardanoAddressPtr address,
      base::expected<cardano_rpc::UnspentOutputs, std::string> utxos);

  raw_ref<CardanoWalletService> cardano_wallet_service_;
  std::string chain_id_;
  std::vector<mojom::CardanoAddressPtr> addresses_;
  bool requests_sent_ = false;

  std::optional<std::string> error_;
  std::optional<EpochParameters> result_;
  Callback callback_;
  base::WeakPtrFactory<GetLatestEpochParametersTask> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_GET_LATEST_EPOCH_PARAMETERS_TASK_H_
