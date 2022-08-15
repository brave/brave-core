/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_ENDPOINT_H_

#include <string>
#include <vector>

#include "base/location.h"
#include "base/types/expected.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {

class Endpoint {
 public:
  virtual ~Endpoint();

  base::expected<type::UrlRequestPtr, std::string> Request() const;

  void Send(type::UrlRequestPtr, client::LoadURLCallback) const;

 protected:
  explicit Endpoint(LedgerImpl*);

  virtual base::expected<std::string, base::Location> Url() const = 0;

  virtual type::UrlMethod Method() const;

  virtual base::expected<std::vector<std::string>, base::Location> Headers()
      const;

  virtual base::expected<std::string, base::Location> Content() const;

  virtual std::string ContentType() const;

  virtual bool SkipLog() const;

  virtual uint32_t LoadFlags() const;

  LedgerImpl* ledger_;  // NOT OWNED

 private:
  uint32_t RetryCountOnRateLimiting() const;
};

}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_ENDPOINT_H_
