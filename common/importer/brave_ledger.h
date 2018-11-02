/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_
#define BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_

#include <string>
#include <vector>

struct BraveLedger {
  BraveLedger();
  BraveLedger(const BraveLedger& other);
  ~BraveLedger();

  std::vector<uint8_t> wallet_seed;
  std::string passphrase;
};

#endif  // BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_
