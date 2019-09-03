/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

const char kCardName[] = "Brave Browser";
const char kAnonID[] = "anonymous";

struct UpdateCard {
  std::string label;
  int32_t position;
  bool starred;

  UpdateCard();
  ~UpdateCard();
};

using UpdateCardCallback = std::function<void(const ledger::Result)>;

using GetCardAddressesCallback =
    std::function<void(ledger::Result, std::map<std::string, std::string>)>;

class UpholdCard {
 public:
  explicit UpholdCard(bat_ledger::LedgerImpl* ledger, Uphold* uphold);

  ~UpholdCard();

  void CreateIfNecessary(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback);

  void CreateAnonAddressIfNecessary(
      ledger::ExternalWalletPtr wallet,
      CreateAnonAddressCallback callback);

 private:
  void OnCreateIfNecessary(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const ledger::ExternalWallet& wallet,
    CreateCardCallback callback);

  void Create(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback);

  void OnCreate(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const ledger::ExternalWallet& wallet,
    CreateCardCallback callback);

  void OnCreateUpdate(
    const ledger::Result result,
    const std::string& address,
    CreateCardCallback callback);

  void Update(
    ledger::ExternalWalletPtr wallet,
    const UpdateCard& card,
    UpdateCardCallback callback);

  void OnUpdate(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    UpdateCardCallback callback);

  void GetCardAddresses(
      ledger::ExternalWalletPtr wallet,
      GetCardAddressesCallback callback);

  std::map<std::string, std::string> ParseGetCardAddressResponse(
      const std::string& response);

  void OnGetCardAddresses(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      GetCardAddressesCallback callback);

  void OnCreateAnonAddressGet(
      ledger::Result result,
      std::map<std::string, std::string> addresses,
      const ledger::ExternalWallet& wallet);

  void OnCreateAnonAddressIfNecessary(
      ledger::Result result,
      std::map<std::string, std::string> addresses,
      const ledger::ExternalWallet& wallet,
      CreateAnonAddressCallback callback);

  void CreateAnonAddress(
      ledger::ExternalWalletPtr wallet,
      CreateAnonAddressCallback callback);

  void OnCreateAnonAddress(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      CreateAnonAddressCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Uphold* uphold_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_CARD_H_
