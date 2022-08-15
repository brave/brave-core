/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_BASE_H_

#include <string>
#include <vector>

#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {
class EnsGetEthAddrTask;

class JsonRpcServiceBase {
 public:
  using RequestIntermediateCallback = base::OnceCallback<void(
      int http_code,
      const std::string& response,
      const base::flat_map<std::string, std::string>& headers)>;

  virtual ~JsonRpcServiceBase() = 0;

  virtual void OnEnsResolverTaskDone(EnsGetEthAddrTask* task,
                                     std::vector<uint8_t> resolved_result,
                                     mojom::ProviderError error,
                                     std::string error_message) = 0;

  virtual void RequestInternal(
      const std::string& json_payload,
      bool auto_retry_on_network_change,
      const GURL& network_url,
      RequestIntermediateCallback callback,
      api_request_helper::APIRequestHelper::ResponseConversionCallback
          conversion_callback) = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_BASE_H_
