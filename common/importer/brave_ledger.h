/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_
#define BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_

#include <string>
#include <vector>
#include <list>
#include <map>

struct BraveLedger {
  BraveLedger();
  BraveLedger(const BraveLedger& other);
  ~BraveLedger();

  std::vector<uint8_t> wallet_seed;
  std::string passphrase;
  bool clobber_wallet;
  std::list<std::string> excluded_publishers;
  std::map<std::string, unsigned int> pinned_publishers;

  // NOTE: if we need to read more data from session-store-1,
  // we may consider moving this to brave_importer.h (or similar)
  struct SessionStoreSettings {
    struct PaymentSettings {
      bool allow_media_publishers;
      bool allow_non_verified;
      bool enabled;
      double contribution_amount;
      uint64_t min_visit_time;
      unsigned int min_visits;
    } payments;
  } settings;
};

#endif  // BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_
