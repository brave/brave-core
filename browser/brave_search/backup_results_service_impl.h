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

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/brave_search/browser/backup_results_metrics.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "content/public/browser/navigation_controller.h"
#include "net/http/http_request_headers.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "url/gurl.h"

class PrefService;
class Profile;

namespace gfx {
class Size;
}  // namespace gfx

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
  static void RecordLastViewSize(PrefService* local_state,
                                 const gfx::Size& size);

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
  void HandleWebContentsDidFinishLoad(const content::WebContents* web_contents,
                                      const GURL& url);

  base::WeakPtr<BackupResultsService> GetWeakPtr() override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

  // KeyedService:
  void Shutdown() override;

 private:
  struct PendingRequest {
    PendingRequest(std::unique_ptr<content::WebContents> web_contents,
                   std::optional<net::HttpRequestHeaders> headers,
                   Profile* otr_profile,
                   const GURL& target_url,
                   BackupResultsCallback callback);
    ~PendingRequest();

    std::optional<net::HttpRequestHeaders> headers;
    BackupResultsCallback callback;

    std::unique_ptr<content::WebContents> web_contents;

    raw_ptr<Profile> otr_profile;
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory;
    std::unique_ptr<network::SimpleURLLoader> simple_url_loader;

    // The original target search URL. Used to derive the origin to navigate to
    // and to extract the search query that is typed into the page.
    GURL target_url;
    // The search query (with '+' replaced by spaces) typed into the page.
    std::u16string query;
    // Index of the next character in `query` to type.
    size_t query_char_index = 0;
    // True once the input simulation has been scheduled for this request.
    bool typing_started = false;

    size_t initial_request_started = false;
    size_t requests_loaded = 0;
    int last_response_code = -1;
    base::OneShotTimer timeout_timer;
    // Drives the delayed per-character typing and Enter key press.
    base::OneShotTimer input_timer;
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

  void MaybeApplyUserAgentOverride(
      content::WebContents& web_contents,
      content::NavigationController::LoadURLParams& load_url_params);

  // Returns the origin root (scheme + host, path "/") of `target_url`, or an
  // empty/invalid GURL if it cannot be derived.
  GURL GetOriginUrl(const GURL& target_url);

  // Schedules the initial randomized delay before typing begins. Called once
  // the origin page has finished loading.
  void StartInputSimulation(PendingRequestList::iterator pending_request);

  // Types the next query character into the focused page, scheduling the
  // subsequent character with a randomized delay, then presses Enter once all
  // characters have been typed.
  void TypeNextCharacter(PendingRequestList::iterator pending_request);

  // Forwards a single character keystroke (RawKeyDown + Char + KeyUp) to the
  // web contents.
  void ForwardCharacter(content::WebContents& web_contents, char16_t character);

  // Forwards an Enter key press to the web contents.
  void PressEnter(content::WebContents& web_contents);

  net::HttpRequestHeaders GetExtraHeaders(
      const std::optional<net::HttpRequestHeaders>& request_headers);

  // Returns true if the daily request limit has been reached, false otherwise.
  bool UpdateDailyRequestCount();

  raw_ptr<Profile> profile_;
  raw_ptr<PrefService> local_state_;

  // Cached on first use; nullopt if param is absent/invalid.
  std::optional<blink::UserAgentOverride> ua_override_;
  std::optional<net::HttpRequestHeaders> feature_headers_;

  PendingRequestList pending_requests_;

  BackupResultsMetrics backup_results_metrics_;

  base::WeakPtrFactory<BackupResultsServiceImpl> weak_ptr_factory_{this};
};

}  // namespace brave_search

#endif  // BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_SERVICE_IMPL_H_
