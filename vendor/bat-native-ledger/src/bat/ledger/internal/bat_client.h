/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_CLIENT_H_
#define BRAVELEDGER_BAT_CLIENT_H_

#include <string>
#include <vector>
#include <map>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/publisher_info.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_bat_client {

class BatClient {
 public:
  explicit BatClient(bat_ledger::LedgerImpl* ledger);
  ~BatClient();


  void getGrants(const std::string& lang, const std::string& forPaymentId);

  void setGrant(const std::string& captchaResponse,
                const std::string& promotionId);

  void getGrantCaptcha(
      const std::vector<std::string>& headers);

 private:
  void getGrantCaptchaCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void getGrantsCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void setGrantCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_bat_client

#endif  // BRAVELEDGER_BAT_CLIENT_H_
