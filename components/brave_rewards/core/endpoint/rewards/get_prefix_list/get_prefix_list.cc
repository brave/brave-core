/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/rewards/get_prefix_list/get_prefix_list.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::rewards {

GetPrefixList::GetPrefixList(RewardsEngine& engine) : engine_(engine) {}

GetPrefixList::~GetPrefixList() = default;

std::string GetPrefixList::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .rewards_url()
      .Resolve("/publishers/prefix-list")
      .spec();
}

mojom::Result GetPrefixList::CheckStatusCode(const int status_code) {
  if (status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void GetPrefixList::Request(GetPrefixListCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kBasic,
      base::BindOnce(&GetPrefixList::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void GetPrefixList::OnRequest(GetPrefixListCallback callback,
                              mojom::UrlResponsePtr response) {
  DCHECK(response);

  if (CheckStatusCode(response->status_code) != mojom::Result::OK ||
      response->body.empty()) {
    engine_->LogError(FROM_HERE)
        << "Invalid server response for publisher prefix list";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  std::move(callback).Run(mojom::Result::OK, response->body);
}

}  // namespace brave_rewards::internal::endpoint::rewards
