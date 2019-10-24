/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ATTESTATION_ATTESTATION_IMPL_H_
#define BRAVELEDGER_ATTESTATION_ATTESTATION_IMPL_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/attestation/attestation.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_attestation {

class AttestationDesktop;
class AttestationAndroid;
class AttestationIOS;

class AttestationImpl : public Attestation {
 public:
  explicit AttestationImpl(bat_ledger::LedgerImpl* ledger);
  ~AttestationImpl() override;

  void Start(const std::string& payload, StartCallback callback) override;

  void Confirm(
      const std::string& solution,
      ConfirmCallback callback) override;

 private:
  std::unique_ptr<Attestation> instance_;
};

}  // namespace braveledger_attestation
#endif  // BRAVELEDGER_ATTESTATION_ATTESTATION_IMPL_H_
