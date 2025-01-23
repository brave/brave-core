// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_SERVICE_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

namespace brave_search {

// Fetches search results from a backup search provider,
// for use in Brave Search fallback mixing (GMix) and Web Discovery Project.
//
// Each request will use an OTR profile for temporarily storing cookies, etc.
//
// There are three modes of operation for this service:
// i)   If `features::kBackupResultsFullRender` is disabled, the initial search
//      page will be rendered, and the actual search engine results page will be
//      fetched.
// ii)  If `features::kBackupResultsFullRender` is enabled, the initial search
//      page and the actual search engine results page will be rendered.
// iii) If a cookie header value is provided in `FetchBackupResults`, the actual
//      search engine result page will be directly fetched, with no rendering.
class BackupResultsService : public KeyedService {
 public:
  struct BackupResults {
    BackupResults(int final_status_code, std::string html);
    ~BackupResults();
    int final_status_code;
    std::string html;
  };

  using BackupResultsCallback =
      base::OnceCallback<void(std::optional<BackupResults>)>;

  ~BackupResultsService() override = default;

  virtual void FetchBackupResults(
      const GURL& url,
      std::optional<net::HttpRequestHeaders> headers,
      BackupResultsCallback callback) = 0;

  // Called by BackupResultsNavigationThrottle. Returns true if request should
  // continue.
  virtual bool HandleWebContentsStartRequest(
      const content::WebContents* web_contents,
      const GURL& url) = 0;

  virtual base::WeakPtr<BackupResultsService> GetWeakPtr() = 0;
};

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_SERVICE_H_
