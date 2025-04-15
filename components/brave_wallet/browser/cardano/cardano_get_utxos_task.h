/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_GET_UTXOS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_GET_UTXOS_TASK_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/common/cardano_address.h"

namespace brave_wallet {
class CardanoWalletService;

// This class implements `CardanoWalletService::GetUtxos` logic of fetching all
// utxos associated with `addresses`.
class GetCardanoUtxosTask {
 public:
  using UtxoMap = std::map<CardanoAddress, cardano_rpc::UnspentOutputs>;
  using Callback =
      base::OnceCallback<void(base::expected<UtxoMap, std::string>)>;

  GetCardanoUtxosTask(CardanoWalletService& cardano_wallet_service,
                      const std::string& chain_id,
                      std::vector<CardanoAddress> addresses);
  ~GetCardanoUtxosTask();

  void Start(Callback callback);

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();
  void StopWithError(std::string error_string);

  void MaybeSendRequests();
  void OnGetUtxoList(
      CardanoAddress address,
      base::expected<cardano_rpc::UnspentOutputs, std::string> utxos);

  raw_ref<CardanoWalletService> cardano_wallet_service_;
  std::string chain_id_;
  std::vector<CardanoAddress> addresses_;
  bool requests_sent_ = false;

  UtxoMap utxos_;
  std::optional<UtxoMap> result_;
  Callback callback_;
  base::WeakPtrFactory<GetCardanoUtxosTask> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_GET_UTXOS_TASK_H_
