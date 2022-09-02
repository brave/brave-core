/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_REQUEST_POST_CONNECT_POST_CONNECT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_REQUEST_POST_CONNECT_POST_CONNECT_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "bat/ledger/internal/request/request_builder.h"
#include "bat/ledger/mojom_structs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {
class LedgerImpl;

namespace request {

class PostConnect : public RequestBuilder {
 public:
  using Callback = base::OnceCallback<void(type::Result)>;
  static void OnResponse(Callback, const type::UrlResponse&);

  explicit PostConnect(LedgerImpl*);
  ~PostConnect() override;

 protected:
  virtual const char* Path() const = 0;

 private:
  absl::optional<std::string> Url() const override;
  absl::optional<std::vector<std::string>> Headers() const override;
  std::string ContentType() const override;
};

}  // namespace request
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_REQUEST_POST_CONNECT_POST_CONNECT_H_
