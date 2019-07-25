/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_GRANTS_H_
#define BRAVELEDGER_GRANTS_H_

#include <string>
#include <vector>
#include <map>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/bat_helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_grant {

class Grants {
 public:
  explicit Grants(bat_ledger::LedgerImpl* ledger);
  ~Grants();


  void FetchGrants(
      const std::string& lang,
      const std::string& forPaymentId,
      ledger::FetchGrantsCallback callback);

  void SetGrant(const std::string& captchaResponse,
                const std::string& promotionId);

  void GetGrantCaptcha(const std::vector<std::string>& headers);

 private:
  void GetGrantCaptchaCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void GetGrantsCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::FetchGrantsCallback callback);

  void SetGrantCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_grant

#endif  // BRAVELEDGER_GRANTS_H_
