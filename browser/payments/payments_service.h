/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

namespace braveledger_ledger {
class Ledger;
}

namespace payments {

class PaymentsService : public KeyedService {
 public:
  PaymentsService();
  ~PaymentsService() override;

  // KeyedService:
  void Shutdown() override;

  void CreateWallet();

 private:
  std::unique_ptr<braveledger_ledger::Ledger> ledger_;

  DISALLOW_COPY_AND_ASSIGN(PaymentsService);
};

}  // namespace history

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_
