/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_CREDENTIALS_REDEEM_H_
#define BRAVELEDGER_CREDENTIALS_CREDENTIALS_REDEEM_H_

#include <vector>
#include <string>

#include "bat/ledger/ledger.h"

namespace braveledger_credentials {

struct CredentialsRedeem {
  CredentialsRedeem();
  CredentialsRedeem(const CredentialsRedeem& info);
  ~CredentialsRedeem();

  std::string publisher_key;
  ledger::RewardsType type;
  ledger::ContributionProcessor processor;
  std::vector<ledger::UnblindedToken> token_list;
  std::string order_id;
};

}  // namespace braveledger_credentials

#endif  // BRAVELEDGER_CREDENTIALS_CREDENTIALS_REDEEM_H_
