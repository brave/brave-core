/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/patch_card/patch_card.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::uphold {

PatchCard::PatchCard(RewardsEngineImpl& engine) : engine_(engine) {}

PatchCard::~PatchCard() = default;

std::string PatchCard::GetUrl(const std::string& address) const {
  return GetServerUrl("/v0/me/cards/" + address);
}

std::string PatchCard::GeneratePayload() const {
  base::Value::Dict settings;
  settings.Set("position", 1);
  settings.Set("starred", true);

  base::Value::Dict payload;
  payload.Set("settings", std::move(settings));

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

mojom::Result PatchCard::CheckStatusCode(int status_code) const {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Unauthorized access");
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PatchCard::Request(const std::string& token,
                        const std::string& address,
                        PatchCardCallback callback) const {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(address);
  request->content = GeneratePayload();
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::PATCH;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PatchCard::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PatchCard::OnRequest(PatchCardCallback callback,
                          mojom::UrlResponsePtr response) const {
  DCHECK(response);
  std::move(callback).Run(CheckStatusCode(response->status_code));
}

}  // namespace brave_rewards::internal::endpoint::uphold
