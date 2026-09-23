/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/renderer/backup_results_url_loader_throttle.h"

#include <algorithm>
#include <string_view>

#include "brave/components/brave_search/common/features.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_request.h"

namespace brave_search {

namespace {

constexpr std::string_view kFetchPrefixes[] = {"/gen_204", "/client_204",
                                               "/complete/s"};

constexpr std::string_view kCosmeticExtensions[] = {".png", ".woff2", ".ico"};

constexpr std::string_view kCosmeticPrefixes[] = {"/images", "/shopping",
                                                  "/favicon", "/grass"};

}  // namespace

BackupResultsURLLoaderThrottle::BackupResultsURLLoaderThrottle() = default;
BackupResultsURLLoaderThrottle::~BackupResultsURLLoaderThrottle() = default;

void BackupResultsURLLoaderThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  std::string_view path = request->url.path();

  if (path == "/" || path.starts_with("/search")) {
    return;
  }

  if (std::ranges::any_of(kFetchPrefixes, [&](std::string_view prefix) {
        return path.starts_with(prefix);
      })) {
    if (!features::kBackupResultsAllowFetches.Get()) {
      delegate_->CancelWithError(net::ERR_FAILED);
    }
    return;
  }

  if (std::ranges::any_of(
          kCosmeticExtensions,
          [&](std::string_view ext) { return path.ends_with(ext); }) ||
      std::ranges::any_of(kCosmeticPrefixes, [&](std::string_view prefix) {
        return path.starts_with(prefix);
      })) {
    if (!features::kBackupResultsAllowCosmeticAssets.Get()) {
      delegate_->CancelWithError(net::ERR_FAILED);
    }
    return;
  }

  if (!features::kBackupResultsAllowUnclassifiedRequests.Get()) {
    delegate_->CancelWithError(net::ERR_FAILED);
  }
}

}  // namespace brave_search
