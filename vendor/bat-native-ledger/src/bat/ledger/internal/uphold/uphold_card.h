/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class UpholdServer;
}

namespace uphold {

const char kCardName[] = "Brave Browser";

struct UpdateCard {
  std::string label;
  int32_t position;
  bool starred;

  UpdateCard();
  ~UpdateCard();
};

using GetCardAddressesCallback =
    std::function<void(type::Result, std::map<std::string, std::string>)>;

class UpholdCard {
 public:
  explicit UpholdCard(LedgerImpl* ledger);

  ~UpholdCard();

  void CreateIfNecessary(CreateCardCallback callback);

 private:
  void OnCreateIfNecessary(
      const type::Result result,
      const std::string& id,
      CreateCardCallback callback);

  void Create(CreateCardCallback callback);

  void OnCreate(
      const type::Result result,
      const std::string& id,
      CreateCardCallback callback);

  void OnCreateUpdate(
      const type::Result result,
      const std::string& address,
      CreateCardCallback callback);

  void Update(
      const UpdateCard& card,
      ledger::ResultCallback callback);

  void OnUpdate(
      const type::Result result,
      ledger::ResultCallback callback);

  std::map<std::string, std::string> ParseGetCardAddressResponse(
      const std::string& response);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::UpholdServer> uphold_server_;
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_
