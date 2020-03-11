/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/request_sku.h"
#include "bat/ledger/internal/request/request_util.h"

namespace {

std::string GetTransactionSuffix(const ledger::SKUTransactionType type) {
  switch (type) {
    case ledger::SKUTransactionType::NONE: {
      return "";
    }
    case ledger::SKUTransactionType::UPHOLD: {
      return "uphold";
    }
    case ledger::SKUTransactionType::ANONYMOUS_CARD: {
      return "anonymousCard";
    }
  }
}

}  // namespace

namespace braveledger_request_util {

std::string GetCreateOrderURL() {
  return BuildUrl("/orders", PREFIX_V1, ServerTypes::kPromotion);
}

std::string GetOrderCredentialsURL(
    const std::string& order_id,
    const std::string& item_id) {
  std::string item_path = "";
  if (!item_id.empty()) {
    item_path = base::StringPrintf("/%s", item_id.c_str());
  }

  const std::string path = base::StringPrintf(
      "/orders/%s/credentials%s",
      order_id.c_str(),
      item_path.c_str());

  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

std::string GetCreateTransactionURL(
    const std::string& order_id,
    const ledger::SKUTransactionType type) {
  const std::string suffix = GetTransactionSuffix(type);
  const std::string path = base::StringPrintf(
      "/orders/%s/transactions/%s",
      order_id.c_str(),
      suffix.c_str());

  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

}  // namespace braveledger_request_util
