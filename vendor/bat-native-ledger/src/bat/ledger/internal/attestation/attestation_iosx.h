/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ATTESTATION_ATTESTATION_IOS_H_
#define BRAVELEDGER_ATTESTATION_ATTESTATION_IOS_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "bat/ledger/internal/attestation/attestation.h"
#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"

namespace ledger {
class LedgerImpl;

namespace attestation {

class AttestationIOS : public Attestation {
 public:
  explicit AttestationIOS(LedgerImpl* ledger);
  ~AttestationIOS() override;

  void Start(const std::string& payload, StartCallback callback) override;

  void Confirm(
      const std::string& solution,
      ConfirmCallback callback) override;

 private:
  std::string ParseStartPayload(
      const std::string& response);

  type::Result ParseClaimSolution(
      const std::string& response,
      std::string* nonce,
      std::string* blob,
      std::string* signature);

  void OnStart(
      const type::Result result,
      const std::string& nonce,
      StartCallback callback);

  void OnConfirm(
      const type::Result result,
      ConfirmCallback callback);

  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace attestation
}  // namespace ledger
#endif  // BRAVELEDGER_ATTESTATION_ATTESTATION_IOS_H_
