/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_URL_LOADER_
#define BAT_LEDGER_LEDGER_URL_LOADER_

#include <string>

#include "bat/ledger/export.h"

namespace ledger {

class LEDGER_EXPORT LedgerURLLoader {
 public:
  virtual ~LedgerURLLoader() = default;
  virtual void Start() = 0;
  virtual uint64_t request_id() = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_URL_LOADER_
