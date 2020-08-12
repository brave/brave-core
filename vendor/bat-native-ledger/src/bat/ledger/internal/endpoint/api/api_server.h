/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_API_API_SERVER_H_
#define BRAVELEDGER_ENDPOINT_API_API_SERVER_H_

#include <memory>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/endpoint/api/get_parameters/get_parameters.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {

class APIServer {
 public:
  explicit APIServer(bat_ledger::LedgerImpl* ledger);
  ~APIServer();

  api::GetParameters* get_parameters() const;

 private:
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<api::GetParameters> get_parameters_;
};

}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_API_API_SERVER_H_
