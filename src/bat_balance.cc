/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_balance.h"
#include "static_values.h"

namespace bat_balance {

BatBalance::BatBalance() {
}

BatBalance::~BatBalance() {

}

std::string BatBalance::buildURL(const std::string& path, const std::string& prefix) {
  std::string url;
  if (ledger::g_isProduction) {
    url = BALANCE_PRODUCTION_SERVER;
  } else {
    url = BALANCE_STAGING_SERVER;
  }

  return url + prefix + path;
}

void BatBalance::getWalletProperties(const std::string& paymentInfo, BatHelper::FetchCallback callback,
  const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
    batClientWebRequest_.run(buildURL((std::string)WALLET_PROPERTIES + paymentInfo + WALLET_PROPERTIES_END, ""),
      callback, std::vector<std::string>(), "", "", extraData, URL_METHOD::GET);
}

}
