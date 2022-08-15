/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/post_connect/bitflyer/post_connect_bitflyer.h"

#include "base/json/json_writer.h"
#include "bat/ledger/internal/logging/logging.h"

namespace ledger::endpoint::connect {

PostConnectBitflyer::PostConnectBitflyer(LedgerImpl* ledger,
                                         const std::string& linking_info)
    : PostConnect(ledger), linking_info_(linking_info) {}

PostConnectBitflyer::~PostConnectBitflyer() = default;

base::expected<std::string, base::Location> PostConnectBitflyer::Content()
    const {
  base::Value::Dict content;
  content.Set("linkingInfo", linking_info_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    BLOG(0, "Failed to write content to JSON!");
    return base::unexpected(base::Location::Current());
  }

  return json;
}

const char* PostConnectBitflyer::Path() const {
  return "/v3/wallet/bitflyer/%s/claim";
}

}  // namespace ledger::endpoint::connect
