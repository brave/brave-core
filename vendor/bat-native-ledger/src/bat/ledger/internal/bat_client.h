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

  void registerPersona();

  void requestCredentialsCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void registerPersonaCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  std::string getWalletPassphrase() const;

  void WalletPropertiesCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::OnWalletPropertiesCallback callback);

  void recoverWallet(const std::string& passPhrase);

  void getGrants(const std::string& lang, const std::string& forPaymentId);

  void setGrant(const std::string& captchaResponse,
                const std::string& promotionId);

  void getGrantCaptcha(
      const std::vector<std::string>& headers);

  void GetWalletProperties(ledger::OnWalletPropertiesCallback callback);

  void continueRecover(int result,
                       size_t* written,
                       const std::vector<uint8_t>& newSeed);

  void OnNicewareListLoaded(const std::string& pass_phrase,
                            ledger::Result result,
                            const std::string& data);

  void GetAddressesForPaymentId(ledger::WalletAddressesCallback callback);

  void CreateWalletIfNecessary();

  ledger::WalletInfo WalletPropertiesToWalletInfo(
    const braveledger_bat_helper::WALLET_PROPERTIES_ST& properties);

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

  void recoverWalletPublicKeyCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void recoverWalletCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string& paymentId);

  std::string getAnonizeProof(const std::string& registrarVK,
                              const std::string& id,
                              std::string* preFlight);

  void GetAddressesForPaymentIdCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::WalletAddressesCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_bat_client

#endif  // BRAVELEDGER_BAT_CLIENT_H_
