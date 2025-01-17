/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_FALLBACK_HOST_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_FALLBACK_HOST_H_

#include <list>
#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_search/common/brave_search_fallback.mojom.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_search {

class BraveSearchFallbackHost final
    : public brave_search::mojom::BraveSearchFallback {
 public:
  BraveSearchFallbackHost(const BraveSearchFallbackHost&) = delete;
  BraveSearchFallbackHost& operator=(const BraveSearchFallbackHost&) = delete;
  explicit BraveSearchFallbackHost(
      scoped_refptr<network::SharedURLLoaderFactory> factory);
  ~BraveSearchFallbackHost() override;

  void FetchBackupResults(const std::string& query_string,
                          const std::string& lang,
                          const std::string& country,
                          const std::string& geo,
                          bool filter_explicit_results,
                          int page_index,
                          const std::optional<std::string>& cookie_header_value,
                          FetchBackupResultsCallback callback) override;

  static GURL GetBackupResultURL(const GURL& baseURL,
                                 const std::string& query,
                                 const std::string& lang,
                                 const std::string& country,
                                 const std::string& geo,
                                 bool filter_explicit_results,
                                 int page_index);
  static void SetBackupProviderForTest(const GURL&);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  using URLRequestCallback =
      base::OnceCallback<void(const int,
                              const std::string&,
                              const std::map<std::string, std::string>&)>;

  void OnURLLoaderComplete(
      SimpleURLLoaderList::iterator iter,
      BraveSearchFallbackHost::FetchBackupResultsCallback callback,
      const std::unique_ptr<std::string> response_body);
  SimpleURLLoaderList url_loaders_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::WeakPtrFactory<BraveSearchFallbackHost> weak_factory_;
};

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_FALLBACK_HOST_H_
