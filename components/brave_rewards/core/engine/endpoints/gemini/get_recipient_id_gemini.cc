/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/endpoints/gemini/get_recipient_id_gemini.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/engine/endpoint/gemini/post_recipient_id/post_recipient_id_gemini.h"
#include "brave/components/brave_rewards/core/engine/rewards_engine.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

namespace brave_rewards::internal::endpoints {
using Error = GetRecipientIDGemini::Error;
using Result = GetRecipientIDGemini::Result;

namespace {

Result ParseBody(RewardsEngine& engine, const std::string& body) {
  auto value = base::JSONReader::ReadList(body);
  if (!value) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (auto& item : *value) {
    auto* pair = item.GetIfDict();
    if (!pair) {
      engine.LogError(FROM_HERE) << "Failed to parse body";
      return base::unexpected(Error::kFailedToParseBody);
    }

    auto* label = pair->FindString("label");
    auto* recipient_id = pair->FindString("recipient_id");

    if (!label || !recipient_id) {
      engine.LogError(FROM_HERE) << "Failed to parse body";
      return base::unexpected(Error::kFailedToParseBody);
    }

    if (*label == endpoint::gemini::PostRecipientId::kRecipientLabel) {
      return std::move(*recipient_id);
    }
  }

  return "";
}

}  // namespace

// static
Result GetRecipientIDGemini::ProcessResponse(
    RewardsEngine& engine,
    const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(engine, response.body);
    default:
      engine.LogError(FROM_HERE)
          << "Unexpected status code! (HTTP " << response.status_code << ')';
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

GetRecipientIDGemini::GetRecipientIDGemini(RewardsEngine& engine,
                                           std::string&& token)
    : RequestBuilder(engine), token_(std::move(token)) {}

GetRecipientIDGemini::~GetRecipientIDGemini() = default;

std::optional<std::string> GetRecipientIDGemini::Url() const {
  return engine_->Get<EnvironmentConfig>()
      .gemini_api_url()
      .Resolve("/v1/payments/recipientIds")
      .spec();
}

mojom::UrlMethod GetRecipientIDGemini::Method() const {
  return mojom::UrlMethod::GET;
}

std::optional<std::vector<std::string>> GetRecipientIDGemini::Headers(
    const std::string&) const {
  return std::vector<std::string>{"Authorization: Bearer " + token_};
}

}  // namespace brave_rewards::internal::endpoints
