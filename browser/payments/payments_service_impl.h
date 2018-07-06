/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_

#include <memory>

#include "brave/browser/payments/payments_service.h"

namespace braveledger_ledger {
class Ledger;
}

namespace payments {

class PaymentsServiceImpl : public PaymentsService {
 public:
  PaymentsServiceImpl();
  ~PaymentsServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  void CreateWallet() override;

 private:
  std::unique_ptr<braveledger_ledger::Ledger> ledger_;

  DISALLOW_COPY_AND_ASSIGN(PaymentsServiceImpl);
};

}  // namespace history

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_
