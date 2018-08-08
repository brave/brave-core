/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_PROMO_HANDLER_
#define BAT_LEDGER_PROMO_HANDLER_

#include <string>

#include "bat/ledger/export.h"

namespace ledger {

LEDGER_EXPORT struct Promo {
  Promo();
  ~Promo();
  Promo(const Promo& properties);

  std::string promotionId;
  double amount;
};

}  // namespace ledger

#endif  // BAT_LEDGER_PROMO_HANDLER_
