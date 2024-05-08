/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_GET_ORDINALS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_GET_ORDINALS_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin_ordinals_rpc_responses.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"

namespace brave_wallet {
class BitcoinWalletService;

class GetOrdinalsTask : public base::RefCountedThreadSafe<GetOrdinalsTask> {
 public:
  using OrdinalsMap =
      std::map<BitcoinOutpoint, bitcoin_ordinals_rpc::OutpointInfo>;
  using GetOrdinalsCallback =
      base::OnceCallback<void(base::expected<OrdinalsMap, std::string>)>;

  GetOrdinalsTask(base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service,
                  const std::string& chain_id,
                  const std::vector<BitcoinOutpoint>& outpoints,
                  GetOrdinalsCallback callback);
  void ScheduleWorkOnTask();

 private:
  friend class base::RefCountedThreadSafe<GetOrdinalsTask>;
  virtual ~GetOrdinalsTask();

  void WorkOnTask();
  void MaybeSendRequests();
  void OnGetOutpointInfo(BitcoinOutpoint outpoint,
                         base::expected<bitcoin_ordinals_rpc::OutpointInfo,
                                        std::string> output_info);

  base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service_;
  std::string chain_id_;
  std::vector<BitcoinOutpoint> pending_outpoints_;
  bool requests_sent_ = false;

  OrdinalsMap ordinals_;
  std::optional<std::string> error_;
  std::optional<OrdinalsMap> result_;
  GetOrdinalsCallback callback_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_GET_ORDINALS_H_
