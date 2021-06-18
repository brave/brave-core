/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_UPDATE_CARD_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_UPDATE_CARD_H_

#include <string>

namespace ledger {
namespace uphold {

struct UpdateCard {
  std::string label;
  int32_t position;
  bool starred;

  UpdateCard();
  ~UpdateCard();
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_UPDATE_CARD_H_
