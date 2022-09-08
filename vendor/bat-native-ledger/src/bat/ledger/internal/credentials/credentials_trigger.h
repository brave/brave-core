/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_TRIGGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_TRIGGER_H_

#include <vector>
#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
namespace credential {

struct CredentialsTrigger {
  CredentialsTrigger();
  CredentialsTrigger(const CredentialsTrigger& info);
  ~CredentialsTrigger();

  std::string id;
  mojom::CredsBatchType type;
  int size;
  std::vector<std::string> data;
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_TRIGGER_H_
