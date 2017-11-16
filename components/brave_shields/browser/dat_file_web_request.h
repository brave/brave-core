/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_WEB_REQUEST_H_
#define COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_WEB_REQUEST_H_

#include <memory>
#include <string>
#include <utility>

#include "net/url_request/url_fetcher_delegate.h"
#include "base/callback.h"
#include "base/macros.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
}  // namespace net

namespace brave_shields {

class DATFileWebRequest : public net::URLFetcherDelegate {
 public:
  typedef base::Callback<void(bool)> FetchCallback;

  DATFileWebRequest(const std::string &dat_file_name,
    const GURL& url,
    FetchCallback fetch_callback);
  ~DATFileWebRequest() override;

  void Init();

  // Start fetching the URL with the fetcher. The delegate is notified
  // asynchronously when done.  Start may be called more than once in some
  // cases.  If so, subsequent starts will be ignored since the operation is
  // already in progress.
  void Start();

  // URLFetcherDelegate
  // This will be called when the URL has been fetched, successfully or not.
  // Use accessor methods on |source| to get the results.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  GURL url_;
  FetchCallback fetch_callback_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;
  std::string dat_file_name_;

  DISALLOW_COPY_AND_ASSIGN(DATFileWebRequest);
};

}  // namespace brave_shields

#endif  // COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_WEB_REQUEST_H_
