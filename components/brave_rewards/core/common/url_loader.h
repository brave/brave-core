/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_LOADER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_LOADER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Responsible for dispatching URL network requests to the browser, and logging
// both requests and responses.
class URLLoader : public RewardsEngineHelper {
 public:
  explicit URLLoader(RewardsEngineImpl& engine);
  ~URLLoader() override;

  enum class LogLevel {
    // Skips logging for requests and responses.
    kNone,

    // Logs basic request and response info, including the URL, the method,
    // and HTTP response codes.
    kBasic,

    // Logs additional info, including the request and response body, and
    // allowed request headers. Do not use for any requests that may contain
    // access-granting tokens. All requests using this logging level should be
    // carefully reviewed.
    kDetailed
  };

  using LoadCallback = base::OnceCallback<void(mojom::UrlResponsePtr)>;

  // Dispatches the specified URL request to the browser, using the provided
  // logging option.
  void Load(mojom::UrlRequestPtr request,
            LogLevel log_level,
            LoadCallback callback);

  // Returns a value indicating whether the specified request header should be
  // logged when using the `kDetailed` log level.
  static bool ShouldLogRequestHeader(const std::string& header);

 private:
  void LogRequest(const mojom::UrlRequest& request, LogLevel log_level);
  void LogResponse(const mojom::UrlResponse& response, LogLevel log_level);

  void OnResponse(LogLevel log_level,
                  LoadCallback callback,
                  mojom::UrlResponsePtr response);

  base::WeakPtrFactory<URLLoader> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_LOADER_H_
