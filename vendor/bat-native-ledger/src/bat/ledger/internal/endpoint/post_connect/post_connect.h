/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_POST_CONNECT_POST_CONNECT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_POST_CONNECT_POST_CONNECT_H_

#include <string>
#include <vector>

#include "base/location.h"
#include "base/types/expected.h"
#include "bat/ledger/internal/endpoint/endpoint.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {

class PostConnect : public Endpoint {
 public:
  using Callback = base::OnceCallback<void(type::Result)>;
  static void OnResponse(Callback, const type::UrlResponse&);

  explicit PostConnect(LedgerImpl*);
  ~PostConnect() override;

 protected:
  virtual const char* Path() const = 0;

 private:
  base::expected<std::string, base::Location> Url() const override;
  base::expected<std::vector<std::string>, base::Location> Headers()
      const override;
};

}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_POST_CONNECT_POST_CONNECT_H_
