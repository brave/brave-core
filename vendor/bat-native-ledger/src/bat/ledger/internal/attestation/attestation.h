/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ATTESTATION_ATTESTATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ATTESTATION_ATTESTATION_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace attestation {

using StartCallback =
    base::OnceCallback<void(mojom::Result, const std::string&)>;

using ConfirmCallback = base::OnceCallback<void(mojom::Result)>;

class Attestation {
 public:
  explicit Attestation(LedgerImpl* ledger);
  virtual ~Attestation();

  virtual void Start(const std::string& payload, StartCallback callback) = 0;

  virtual void Confirm(
      const std::string& solution,
      ConfirmCallback callback) = 0;

 protected:
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace attestation
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ATTESTATION_ATTESTATION_H_
