/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_RESPONSE_HANDLER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_RESPONSE_HANDLER_H_

#include <utility>

#include "base/callback.h"
#include "base/types/expected.h"
#include "bat/ledger/internal/endpoints/result_for.h"
#include "bat/ledger/internal/logging/logging_util.h"
#include "bat/ledger/mojom_structs.h"

namespace ledger::endpoints {

template <typename Endpoint>
class ResponseHandler {
 public:
  using Value = typename ResultFor<Endpoint>::Value;
  using Error = typename ResultFor<Endpoint>::Error;
  using Result = base::expected<Value, Error>;

 private:
  static void OnResponse(base::OnceCallback<void(Result&&)> callback,
                         const ledger::mojom::UrlResponse& response) {
    ledger::LogUrlResponse(__func__, response);
    std::move(callback).Run(Endpoint::ProcessResponse(response));
  }

  // Note that friend class RequestFor<Endpoint>; is not sufficient due to
  // class hierarchies implementing an endpoint (e.g. PostConnect is the one
  // that derives from ResponseHandler<PostConnect>, but we're passing
  // PostConnectBitflyer, PostConnectGemini and PostConnectUphold to
  // RequestFor<>).
  template <typename>
  friend class RequestFor;
};

}  // namespace ledger::endpoints

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_RESPONSE_HANDLER_H_
