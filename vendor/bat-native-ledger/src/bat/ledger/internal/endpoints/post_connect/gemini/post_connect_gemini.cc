/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoints/post_connect/gemini/post_connect_gemini.h"

#include <utility>

#include "base/json/json_writer.h"
#include "bat/ledger/internal/logging/logging.h"

namespace ledger::endpoints {

PostConnectGemini::PostConnectGemini(LedgerImpl* ledger,
                                     std::string&& linking_info,
                                     std::string&& recipient_id)
    : PostConnect(ledger),
      linking_info_(std::move(linking_info)),
      recipient_id_(std::move(recipient_id)) {}

PostConnectGemini::~PostConnectGemini() = default;

absl::optional<std::string> PostConnectGemini::Content() const {
  if (linking_info_.empty()) {
    BLOG(0, "linking_info_ is empty!");
    return absl::nullopt;
  }

  if (recipient_id_.empty()) {
    BLOG(0, "recipient_id_ is empty!");
    return absl::nullopt;
  }

  base::Value::Dict content;
  content.Set("linking_info", linking_info_);
  content.Set("recipient_id", recipient_id_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    BLOG(0, "Failed to write content to JSON!");
    return absl::nullopt;
  }

  return json;
}

const char* PostConnectGemini::Path() const {
  return "/v3/wallet/gemini/%s/claim";
}

}  // namespace ledger::endpoints
