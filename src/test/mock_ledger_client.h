/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_MOCK_LEDGER_CLIENT_
#define BAT_LEDGER_MOCK_LEDGER_CLIENT_

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

  void CreateWallet() override;

 protected:
  // ledger::LedgerClient
  std::string GenerateGUID() const override;
  void OnWalletInitialized(ledger::Result result) override;
  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id) override;
  void LoadLedgerState(ledger::LedgerCallbackHandler* handler) override;
  void LoadPublisherState(ledger::LedgerCallbackHandler* handler) override;
  void SaveLedgerState(const std::string& ledger_state,
                       ledger::LedgerCallbackHandler* handler) override;
  void SavePublisherState(const std::string& publisher_state,
                          ledger::LedgerCallbackHandler* handler) override;
  uint64_t LoadURL(const std::string& url,
                   const std::vector<std::string>& headers,
                   const std::string& content,
                   const std::string& contentType,
                   const ledger::URL_METHOD& method,
                   ledger::LedgerCallbackHandler* handler) override;
  void RunIOTask(std::unique_ptr<ledger::LedgerTaskRunner> task) override;
  void RunTask(std::unique_ptr<ledger::LedgerTaskRunner> task) override;

  std::unique_ptr<ledger::Ledger> ledger_;
  std::string ledger_state_;
  std::string publisher_state_;
};

}  // namespace history

#endif  //BAT_LEDGER_MOCK_LEDGER_CLIENT_
