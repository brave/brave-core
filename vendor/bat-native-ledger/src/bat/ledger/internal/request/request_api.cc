/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/request_api.h"
#include "bat/ledger/internal/request/request_util.h"

namespace braveledger_request_util {

std::string GetParametersURL(const std::string& currency) {
  std::string query;
  if (!currency.empty()) {
    query = base::StringPrintf("?currency=%s", currency.c_str());
  }

  const std::string path = base::StringPrintf(
      "/parameters%s",
      query.c_str());

  return BuildUrl(path, PREFIX_V1, ServerTypes::kAPI);
}

}  // namespace braveledger_request_util
