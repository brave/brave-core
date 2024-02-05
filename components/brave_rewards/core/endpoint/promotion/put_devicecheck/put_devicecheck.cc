/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/put_devicecheck/put_devicecheck.h"

#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PutDevicecheck::PutDevicecheck(RewardsEngineImpl& engine) : engine_(engine) {}

PutDevicecheck::~PutDevicecheck() = default;

std::string PutDevicecheck::GetUrl(const std::string& nonce) {
  auto url =
      URLHelpers::Resolve(engine_->Get<EnvironmentConfig>().rewards_grant_url(),
                          {"/v1/devicecheck/attestations/", nonce});
  return url.spec();
}

std::string PutDevicecheck::GeneratePayload(const std::string& blob,
                                            const std::string& signature) {
  base::Value::Dict dictionary;
  dictionary.Set("attestationBlob", blob);
  dictionary.Set("signature", signature);
  std::string payload;
  base::JSONWriter::Write(dictionary, &payload);

  return payload;
}

mojom::Result PutDevicecheck::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Invalid solution");
    return mojom::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Failed to verify captcha solution");
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PutDevicecheck::Request(const std::string& blob,
                             const std::string& signature,
                             const std::string& nonce,
                             PutDevicecheckCallback callback) {
  auto url_callback = base::BindOnce(
      &PutDevicecheck::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(nonce);
  request->content = GeneratePayload(blob, signature);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::PUT;

  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void PutDevicecheck::OnRequest(PutDevicecheckCallback callback,
                               mojom::UrlResponsePtr response) {
  DCHECK(response);
  std::move(callback).Run(CheckStatusCode(response->status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
