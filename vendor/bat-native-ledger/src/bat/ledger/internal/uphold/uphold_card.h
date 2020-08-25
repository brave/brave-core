/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
class UpholdServer;
}
}

namespace braveledger_uphold {

const char kCardName[] = "Brave Browser";

struct UpdateCard {
  std::string label;
  int32_t position;
  bool starred;

  UpdateCard();
  ~UpdateCard();
};

using GetCardAddressesCallback =
    std::function<void(ledger::Result, std::map<std::string, std::string>)>;

class UpholdCard {
 public:
  explicit UpholdCard(bat_ledger::LedgerImpl* ledger, Uphold* uphold);

  ~UpholdCard();

  void CreateIfNecessary(CreateCardCallback callback);

 private:
  void OnCreateIfNecessary(
      const ledger::Result result,
      const std::string& id,
      CreateCardCallback callback);

  void Create(CreateCardCallback callback);

  void OnCreate(
      const ledger::Result result,
      const std::string& id,
      CreateCardCallback callback);

  void OnCreateUpdate(
      const ledger::Result result,
      const std::string& address,
      CreateCardCallback callback);

  void Update(
      const UpdateCard& card,
      ledger::ResultCallback callback);

  void OnUpdate(
      const ledger::Result result,
      ledger::ResultCallback callback);

  std::map<std::string, std::string> ParseGetCardAddressResponse(
      const std::string& response);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Uphold* uphold_;  // NOT OWNED
  std::unique_ptr<ledger::endpoint::UpholdServer> uphold_server_;
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_
