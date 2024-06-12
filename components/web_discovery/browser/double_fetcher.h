/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_DOUBLE_FETCHER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_DOUBLE_FETCHER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/web_discovery/browser/request_queue.h"
#include "url/gurl.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace web_discovery {

class DoubleFetcher {
 public:
  using FetchedCallback =
      base::RepeatingCallback<void(const GURL& url,
                                   const base::Value& associated_data,
                                   std::optional<std::string> response_body)>;
  DoubleFetcher(PrefService* profile_prefs,
                network::SharedURLLoaderFactory* shared_url_loader_factory,
                FetchedCallback callback);
  ~DoubleFetcher();

  DoubleFetcher(const DoubleFetcher&) = delete;
  DoubleFetcher& operator=(const DoubleFetcher&) = delete;

  void ScheduleDoubleFetch(const GURL& url, base::Value associated_data);

 private:
  void OnFetchTimer(const base::Value& request_data);
  void OnRequestComplete(GURL url, std::optional<std::string> response_body);
  bool ProcessCompletedRequest(std::optional<std::string>* response_body);

  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  RequestQueue request_queue_;

  FetchedCallback callback_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_DOUBLE_FETCHER_H_
