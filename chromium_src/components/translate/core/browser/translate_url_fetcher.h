/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_URL_FETCHER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_URL_FETCHER_H_

#include "brave/components/translate/core/browser/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#include "../../../../../../components/translate/core/browser/translate_url_fetcher.h"
#else
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "url/gurl.h"

namespace translate {

// Replace TranslateURLFetcher with a dummy implementation to disable it when
// go-translate is not used.
class TranslateURLFetcher {
 public:
  // Callback type for Request().
  using Callback = base::OnceCallback<void(bool, const std::string&)>;

  // Represents internal state if the fetch is completed successfully.
  enum State {
    IDLE,        // No fetch request was issued.
    REQUESTING,  // A fetch request was issued, but not finished yet.
    COMPLETED,   // The last fetch request was finished successfully.
    FAILED,      // The last fetch request was finished with a failure.
  };

  TranslateURLFetcher() {}
  ~TranslateURLFetcher() {}

  int max_retry_on_5xx() { return 0; }

  void set_max_retry_on_5xx(int count) {}

  const std::string& extra_request_header() { return extra_request_header_; }

  void set_extra_request_header(const std::string& header) {}

  // Requests to |url|. |callback| will be invoked when the function returns
  // true, and the request is finished asynchronously.
  // Returns false if the previous request is not finished, or the request
  // is omitted due to retry limitation. |is_incognito| is used during the fetch
  // to determine which variations headers to add.
  bool Request(const GURL& url, Callback callback, bool is_incognito) {
    return false;
  }

  // Gets internal state.
  State state() { return COMPLETED; }

 private:
  // An extra HTTP request header
  std::string extra_request_header_;

  DISALLOW_COPY_AND_ASSIGN(TranslateURLFetcher);
};

}  // namespace translate
#endif

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_URL_FETCHER_H_
