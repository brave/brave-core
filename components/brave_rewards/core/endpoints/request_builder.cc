/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/request_builder.h"

#include <optional>
#include <utility>

#include "brave/components/brave_rewards/core/common/request_signer.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal::endpoints {

RequestBuilder::~RequestBuilder() = default;

std::optional<mojom::UrlRequestPtr> RequestBuilder::Request() const {
  const auto url = Url();
  if (!url) {
    return std::nullopt;
  }

  const auto content = Content();
  if (!content) {
    return std::nullopt;
  }

  auto headers = Headers(*content);
  if (!headers) {
    return std::nullopt;
  }

  auto request =
      mojom::UrlRequest::New(*url, Method(), std::move(*headers), *content,
                             ContentType(), SkipLog(), LoadFlags());

  if (NeedsToBeSigned()) {
    const auto wallet = engine_->wallet()->GetWallet();
    if (!wallet) {
      BLOG(0, "Rewards wallet is null!");
      return std::nullopt;
    }

    auto request_signer = RequestSigner::FromRewardsWallet(*wallet);
    if (!request_signer || !request_signer->SignRequest(*request)) {
      BLOG(0, "Failed to sign request!");
      return std::nullopt;
    }
  }

  return request;
}

RequestBuilder::RequestBuilder(RewardsEngineImpl& engine) : engine_(engine) {}

mojom::UrlMethod RequestBuilder::Method() const {
  return mojom::UrlMethod::POST;
}

std::optional<std::vector<std::string>> RequestBuilder::Headers(
    const std::string&) const {
  return std::vector<std::string>{};
}

std::optional<std::string> RequestBuilder::Content() const {
  return "";
}

std::string RequestBuilder::ContentType() const {
  return "";
}

bool RequestBuilder::SkipLog() const {
  return false;
}

uint32_t RequestBuilder::LoadFlags() const {
  return 0;
}

bool RequestBuilder::NeedsToBeSigned() const {
  return false;
}

}  // namespace brave_rewards::internal::endpoints
