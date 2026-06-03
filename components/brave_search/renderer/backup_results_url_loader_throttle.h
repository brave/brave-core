/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_RENDERER_BACKUP_RESULTS_URL_LOADER_THROTTLE_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_RENDERER_BACKUP_RESULTS_URL_LOADER_THROTTLE_H_

#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace brave_search {

// Throttle for subresource requests made by the backup results WebContents.
// Allows requests to / and /search* unconditionally. Other requests are
// permitted or cancelled based on kBackupResults feature params:
//   allow_fetches           - known paths for fetch-style requests made by the
//   page allow_cosmetic_assets   - image/font/icon extensions and asset path
//   prefixes allow_unclassified_requests - anything not matching the above
//   categories
class BackupResultsURLLoaderThrottle final : public blink::URLLoaderThrottle {
 public:
  BackupResultsURLLoaderThrottle();
  ~BackupResultsURLLoaderThrottle() override;
  BackupResultsURLLoaderThrottle(const BackupResultsURLLoaderThrottle&) =
      delete;
  BackupResultsURLLoaderThrottle& operator=(
      const BackupResultsURLLoaderThrottle&) = delete;

  // blink::URLLoaderThrottle
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
};

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_RENDERER_BACKUP_RESULTS_URL_LOADER_THROTTLE_H_
