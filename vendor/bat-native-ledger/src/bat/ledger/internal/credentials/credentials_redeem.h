/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_REDEEM_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_REDEEM_H_

#include <vector>
#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
namespace credential {

struct CredentialsRedeem {
  CredentialsRedeem();
  CredentialsRedeem(const CredentialsRedeem& info);
  ~CredentialsRedeem();

  std::string publisher_key;
  mojom::RewardsType type;
  mojom::ContributionProcessor processor;
  std::vector<mojom::UnblindedToken> token_list;
  std::string order_id;
  std::string contribution_id;
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_REDEEM_H_
