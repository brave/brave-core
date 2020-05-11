/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/request_publisher.h"
#include "bat/ledger/internal/request/request_util.h"

namespace braveledger_request_util {

std::string GetPublisherListUrl(const uint32_t page) {
  const std::string path = base::StringPrintf(
      "/api/v3/public/channels?page=%d",
      page);

  return BuildUrl(path, "", ServerTypes::kPublisher);
}

}  // namespace braveledger_request_util
