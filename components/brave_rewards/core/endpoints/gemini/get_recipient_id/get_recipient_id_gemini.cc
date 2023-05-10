/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/gemini/get_recipient_id/get_recipient_id_gemini.h"

#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = GetRecipientIDGemini::Error;
using Result = GetRecipientIDGemini::Result;

namespace {

Result ParseBody(const std::string& body) {
  auto value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (auto& item : value->GetList()) {
    auto* pair = item.GetIfDict();
    if (!pair) {
      BLOG(0, "Failed to parse body!");
      return base::unexpected(Error::kFailedToParseBody);
    }

    auto* label = pair->FindString("label");
    auto* recipient_id = pair->FindString("recipient_id");

    if (!label || !recipient_id) {
      BLOG(0, "Failed to parse body!");
      return base::unexpected(Error::kFailedToParseBody);
    }

    if (*label == gemini::kGeminiRecipientIDLabel) {
      return std::move(*recipient_id);
    }
  }

  return "";
}

}  // namespace

// static
Result GetRecipientIDGemini::ProcessResponse(
    const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(response.body);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

GetRecipientIDGemini::GetRecipientIDGemini(LedgerImpl& ledger,
                                           std::string&& token)
    : RequestBuilder(ledger), token_(std::move(token)) {}

GetRecipientIDGemini::~GetRecipientIDGemini() = default;

absl::optional<std::string> GetRecipientIDGemini::Url() const {
  return endpoint::gemini::GetApiServerUrl("/v1/payments/recipientIds");
}

mojom::UrlMethod GetRecipientIDGemini::Method() const {
  return mojom::UrlMethod::GET;
}

absl::optional<std::vector<std::string>> GetRecipientIDGemini::Headers(
    const std::string&) const {
  return endpoint::gemini::RequestAuthorization(token_);
}

}  // namespace brave_rewards::internal::endpoints
