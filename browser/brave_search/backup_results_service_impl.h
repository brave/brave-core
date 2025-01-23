// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_SERVICE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_SERVICE_IMPL_H_

#include <list>
#include <memory>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_search {

class BackupResultsServiceImpl : public BackupResultsService,
                                 public ProfileObserver {
 public:
  explicit BackupResultsServiceImpl(Profile* profile);

  ~BackupResultsServiceImpl() override;
  BackupResultsServiceImpl(const BackupResultsServiceImpl&) = delete;
  BackupResultsServiceImpl& operator=(const BackupResultsServiceImpl&) = delete;

  void FetchBackupResults(const GURL& url,
                          std::optional<net::HttpRequestHeaders> headers,
                          BackupResultsCallback callback) override;

  bool HandleWebContentsStartRequest(const content::WebContents* web_contents,
                                     const GURL& url) override;
  void HandleWebContentsDidFinishNavigation(
      const content::WebContents* web_contents,
      int response_code);
  void HandleWebContentsDidFinishLoad(const content::WebContents* web_contents);

  base::WeakPtr<BackupResultsService> GetWeakPtr() override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

 private:
  struct PendingRequest {
    PendingRequest(std::unique_ptr<content::WebContents> web_contents,
                   std::optional<net::HttpRequestHeaders> headers,
                   Profile* otr_profile,
                   BackupResultsCallback callback);
    ~PendingRequest();

    std::optional<net::HttpRequestHeaders> headers;
    BackupResultsCallback callback;

    std::unique_ptr<content::WebContents> web_contents;

    raw_ptr<Profile> otr_profile;
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory;
    std::unique_ptr<network::SimpleURLLoader> simple_url_loader;

    size_t initial_request_started = false;
    size_t requests_loaded = 0;
    int last_response_code = -1;
    base::OneShotTimer timeout_timer;
  };
  using PendingRequestList = std::list<PendingRequest>;

  PendingRequestList::iterator FindPendingRequest(
      const content::WebContents* web_contents);

  void MakeSimpleURLLoaderRequest(PendingRequestList::iterator pending_request,
                                  const GURL& url);
  void HandleURLLoaderResponse(PendingRequestList::iterator pending_request,
                               std::optional<std::string> html);

  void HandleWebContentsContentExtraction(
      PendingRequestList::iterator pending_request,
      const std::optional<std::string>& content);

  void CleanupAndDispatchResult(PendingRequestList::iterator pending_request,
                                std::optional<BackupResults> result);

  raw_ptr<Profile> profile_;

  PendingRequestList pending_requests_;

  base::WeakPtrFactory<BackupResultsServiceImpl> weak_ptr_factory_{this};
};

}  // namespace brave_search

#endif  // BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_SERVICE_IMPL_H_
