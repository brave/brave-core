/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/request_builder.h"

#include <utility>

namespace brave_rewards::internal::endpoints {

RequestBuilder::~RequestBuilder() = default;

absl::optional<mojom::UrlRequestPtr> RequestBuilder::Request() const {
  const auto url = Url();
  if (!url) {
    return absl::nullopt;
  }

  const auto content = Content();
  if (!content) {
    return absl::nullopt;
  }

  auto headers = Headers(*content);
  if (!headers) {
    return absl::nullopt;
  }

  return mojom::UrlRequest::New(*url, Method(), std::move(*headers), *content,
                                ContentType(), SkipLog(), LoadFlags());
}

RequestBuilder::RequestBuilder(RewardsEngineImpl& engine) : engine_(engine) {}

mojom::UrlMethod RequestBuilder::Method() const {
  return mojom::UrlMethod::POST;
}

absl::optional<std::vector<std::string>> RequestBuilder::Headers(
    const std::string&) const {
  return std::vector<std::string>{};
}

absl::optional<std::string> RequestBuilder::Content() const {
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

}  // namespace brave_rewards::internal::endpoints
