/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_WALLET_INFO_HANDLER_
#define BAT_LEDGER_WALLET_INFO_HANDLER_

#include <string>
#include <map>
#include <vector>

#include "bat/ledger/export.h"

namespace ledger {

struct GRANT {
  std::string altcurrency;
  std::string probi;
  unsigned int expiryTime;
};

LEDGER_EXPORT struct WalletInfo {
  WalletInfo();
  ~WalletInfo();
  WalletInfo(const WalletInfo& info);
  std::string altcurrency_;
  std::string probi_;
  double balance_;
  std::map<std::string, double> rates_;
  std::vector<double> parameters_choices_;
  std::vector<double> parameters_range_;
  unsigned int parameters_days_;
  std::vector<GRANT> grants_;
};

}  // namespace ledger

#endif  // BAT_LEDGER_WALLET_INFO_HANDLER_
