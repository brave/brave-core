/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_BALANCE_H_
#define BAT_BALANCE_H_

#include "bat_client_webrequest.h"
#include "bat_helper.h"
#include <string>

namespace bat_balance
{

class BatBalance {
public:
  BatBalance();
  ~BatBalance();

  void getWalletProperties(const std::string& paymentInfo, BatHelper::FetchCallback callback,
    const FETCH_CALLBACK_EXTRA_DATA_ST& extraData);

private:
  std::string buildURL(const std::string& path, const std::string& prefix);

  bat_client::BatClientWebRequest batClientWebRequest_;
};

}

#endif  // BAT_BALANCE_H_
