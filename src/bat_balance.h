/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_BALANCE_H_
#define BRAVELEDGER_BAT_BALANCE_H_

#include <string>

#include "bat_client_webrequest.h"

namespace braveledger_bat_helper {
struct FETCH_CALLBACK_EXTRA_DATA_ST;
}

namespace braveledger_bat_balance {

class BatBalance {
 public:
  BatBalance() = default;
  ~BatBalance() = default;
  void getWalletProperties(const std::string& paymentInfo,
    braveledger_bat_client_webrequest::FetchCallback callback,
    const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData);

 private:
  std::string buildURL(const std::string& path, const std::string& prefix);
};

}  // namespace braveledger_bat_balance

#endif  // BRAVELEDGER_BAT_BALANCE_H_
