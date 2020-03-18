/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_MOCK_LEDGER_CLIENT_
#define BAT_LEDGER_MOCK_LEDGER_CLIENT_

#include <memory>
#include <vector>
#include <string>

#include "bat/ledger/ledger_client.h"

namespace ledger {
class Ledger;
class LedgerCallbackHandler;
}

namespace bat_ledger {

class MockLedgerClient : public ledger::LedgerClient {
 public:
  MockLedgerClient();
  ~MockLedgerClient() override;

  // KeyedService:
  void Shutdown() override;

  void CreateWallet(ledger::LedgerCallbackHandler* handler) override;

  MOCK_METHOD6(LoadURL,
      void(const std::string&,
          const std::vector<std::string>&,
          const std::string&,
          const std::string&,
          const ledger::UrlMethod,
          ledger::LoadURLCallback));

 protected:
  // ledger::LedgerClient
  std::string GenerateGUID() const override;
  void LoadLedgerState(ledger::LedgerCallbackHandler* handler) override;
  void LoadPublisherState(ledger::LedgerCallbackHandler* handler) override;
  void SaveLedgerState(const std::string& ledger_state,
                       ledger::LedgerCallbackHandler* handler) override;
  void SavePublisherState(const std::string& publisher_state,
                          ledger::LedgerCallbackHandler* handler) override;

  std::unique_ptr<ledger::Ledger> ledger_;
  std::string ledger_state_;
  std::string publisher_state_;
};

}  // namespace bat_ledger

#endif  // BAT_LEDGER_MOCK_LEDGER_CLIENT_
