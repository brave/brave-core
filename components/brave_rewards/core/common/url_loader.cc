/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/url_loader.h"

#include <array>
#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/brave_rewards/core/common/callback_helpers.h"
#include "brave/components/brave_rewards/core/initialization_manager.h"

namespace brave_rewards::internal {

namespace {

mojom::UrlResponsePtr CreateShutdownResponse(const mojom::UrlRequest& request) {
  auto response = mojom::UrlResponse::New();
  response->url = request.url;
  response->status_code = -1;
  return response;
}

}  // namespace

URLLoader::URLLoader(RewardsEngine& engine) : RewardsEngineHelper(engine) {}

URLLoader::~URLLoader() = default;

void URLLoader::Load(mojom::UrlRequestPtr request,
                     LogLevel log_level,
                     LoadCallback callback) {
  CHECK(request);

  if (engine().Get<InitializationManager>().is_shutting_down()) {
    Log(FROM_HERE) << request->url
                   << " will not be fetched: shutdown in progress";
    DeferCallback(FROM_HERE, std::move(callback),
                  CreateShutdownResponse(*request));
    return;
  }

  LogRequest(*request, log_level);

  client().LoadURL(
      std::move(request),
      base::BindOnce(&URLLoader::OnResponse, weak_factory_.GetWeakPtr(),
                     log_level, std::move(callback)));
}

bool URLLoader::ShouldLogRequestHeader(const std::string& header) {
  constexpr std::array allowed_headers{"digest", "signature", "accept",
                                       "content-type"};

  for (auto* name : allowed_headers) {
    if (base::StartsWith(header, name, base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }

  return false;
}

void URLLoader::LogRequest(const mojom::UrlRequest& request,
                           LogLevel log_level) {
  if (log_level == LogLevel::kNone) {
    return;
  }

  auto stream = Log(FROM_HERE);

  stream << "\n[ REQUEST ]"
         << "\n> URL: " << request.url << "\n> Method: " << request.method;

  if (log_level == LogLevel::kBasic) {
    return;
  }

  if (!request.content.empty()) {
    stream << "\n> Content: " << request.content;
  }

  if (!request.content_type.empty()) {
    stream << "\n> Content Type: " << request.content_type;
  }

  for (auto& header : request.headers) {
    if (ShouldLogRequestHeader(header)) {
      stream << "\n> Header " << header;
    }
  }
}

void URLLoader::LogResponse(const mojom::UrlResponse& response,
                            LogLevel log_level) {
  if (log_level == LogLevel::kNone) {
    return;
  }

  std::string result;
  if (!response.error.empty()) {
    result = "Error (" + response.error + ")";
  } else if (response.status_code >= 200 && response.status_code < 300) {
    result = "Success";
  } else {
    result = "Failure";
  }

  auto stream = Log(FROM_HERE);

  stream << "\n[ RESPONSE ]"
         << "\n> URL: " << response.url << "\n> Result: " << result
         << "\n> HTTP Code: " << response.status_code;

  if (log_level == LogLevel::kBasic) {
    return;
  }

  if (!response.body.empty()) {
    stream << "\n> Body: " << response.body;
  }
}

void URLLoader::OnResponse(LogLevel log_level,
                           LoadCallback callback,
                           mojom::UrlResponsePtr response) {
  if (!response->error.empty()) {
    LogError(FROM_HERE) << "Network error: " << response->error;
  }

  LogResponse(*response, log_level);

  std::move(callback).Run(std::move(response));
}

}  // namespace brave_rewards::internal
