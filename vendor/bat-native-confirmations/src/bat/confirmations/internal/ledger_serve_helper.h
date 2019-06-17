/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_LEDGER_SERVE_HELPER_H_
#define BAT_CONFIRMATIONS_INTERNAL_LEDGER_SERVE_HELPER_H_

#include <string>

namespace helper {

class LedgerServe {
 public:
  static std::string GetURL();
};

}  // namespace helper

#endif  // BAT_CONFIRMATIONS_INTERNAL_LEDGER_SERVE_HELPER_H_
