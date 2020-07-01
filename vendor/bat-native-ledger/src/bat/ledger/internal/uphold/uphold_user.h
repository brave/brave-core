/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_USER_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_USER_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

enum UserStatus {
  EMPTY = 0,
  PENDING = 1,
  RESTRICTED = 2,
  BLOCKED = 3,
  OK = 4,
};

struct User {
  std::string name;
  std::string member_at;
  bool verified;
  UserStatus status;
  bool bat_not_allowed;

  User();
  ~User();
};

using GetUserCallback = std::function<void(const ledger::Result, const User)>;

class UpholdUser {
 public:
  explicit UpholdUser(bat_ledger::LedgerImpl* ledger);

  ~UpholdUser();

  void Get(GetUserCallback callback);

 private:
  void OnGet(
      const ledger::UrlResponse& response,
      GetUserCallback callback);

  UserStatus GetStatus(const std::string& status);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_USER_H_
