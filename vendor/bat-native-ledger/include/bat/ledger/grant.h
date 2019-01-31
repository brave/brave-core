/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_GRANT_HANDLER_
#define BAT_LEDGER_GRANT_HANDLER_

#include <string>
#include <map>
#include <vector>

#include "bat/ledger/export.h"

namespace ledger {

LEDGER_EXPORT struct Grant {
  Grant();
  ~Grant();
  Grant(const Grant& properties);

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string altcurrency;
  std::string probi;
  std::string promotionId;
  uint64_t expiryTime;
  std::string type;
};

}  // namespace ledger

#endif  // BAT_LEDGER_GRANT_HANDLER_
