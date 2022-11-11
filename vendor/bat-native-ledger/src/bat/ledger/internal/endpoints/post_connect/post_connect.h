/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_POST_CONNECT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_POST_CONNECT_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/endpoints/request_builder.h"
#include "bat/ledger/internal/endpoints/response_handler.h"
#include "bat/ledger/internal/endpoints/result_for.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_endpoints.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {
class LedgerImpl;

namespace endpoints {

class PostConnect;

template <>
struct ResultFor<PostConnect> {
  using Value = void;
  using Error = mojom::PostConnectError;
};

class PostConnect : public RequestBuilder, public ResponseHandler<PostConnect> {
 public:
  static Result ProcessResponse(const mojom::UrlResponse&);
  static ConnectExternalWalletResult ToConnectExternalWalletResult(
      const Result&);

  explicit PostConnect(LedgerImpl*);
  ~PostConnect() override;

 protected:
  virtual const char* Path() const = 0;

 private:
  absl::optional<std::string> Url() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  std::string ContentType() const override;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_POST_CONNECT_POST_CONNECT_H_
