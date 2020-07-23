/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ATTESTATION_ATTESTATION_DESKTOP_H_
#define BRAVELEDGER_ATTESTATION_ATTESTATION_DESKTOP_H_

#include <map>
#include <string>

#include "bat/ledger/internal/attestation/attestation.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_attestation {

class AttestationDesktop : public Attestation {
 public:
  explicit AttestationDesktop(bat_ledger::LedgerImpl* ledger);
  ~AttestationDesktop() override;

  void Start(const std::string& payload, StartCallback callback) override;

  void Confirm(
      const std::string& solution,
      ConfirmCallback callback) override;

 private:
  void ParseClaimSolution(
      const std::string& response,
      base::Value* result);

  void OnStart(
      const ledger::UrlResponse& response,
      StartCallback callback);

  void DownloadCaptchaImage(
      const ledger::UrlResponse& response,
      StartCallback callback);

  void OnDownloadCaptchaImage(
      const ledger::UrlResponse& response,
      const ledger::UrlResponse& captcha_response,
      StartCallback callback);

  void OnConfirm(
      const ledger::UrlResponse& response,
      ConfirmCallback callback);
};

}  // namespace braveledger_attestation
#endif  // BRAVELEDGER_ATTESTATION_ATTESTATION_DESKTOP_H_
