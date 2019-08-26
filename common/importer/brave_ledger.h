/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_
#define BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_

#include <string>
#include <vector>

struct BravePublisher {
  std::string key;
  bool verified;
  std::string name;
  std::string url;
  std::string provider;
  int pin_percentage;

  BravePublisher();
  BravePublisher(const BravePublisher& other);
  ~BravePublisher();
};

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
  std::string default_search_engine;
  bool use_alternate_private_search_engine;
  bool use_alternate_private_search_engine_tor;
};

struct BraveLedger {
  BraveLedger();
  BraveLedger(const BraveLedger& other);
  ~BraveLedger();

  std::string passphrase;
  std::vector<std::string> excluded_publishers;
  std::vector<BravePublisher> pinned_publishers;
  SessionStoreSettings settings;
};

#endif  // BRAVE_COMMON_IMPORTER_BRAVE_LEDGER_H_
