/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_MACHINE_URL_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_MACHINE_URL_FETCHER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // network

class GURL;

// This only tries to fetch one wayback url at once.
// If client calls Fetch() before OnWaybackURLFetched is called, previous fetch
// request is dropped.
class WaybackMachineURLFetcher final {
 public:
  class Client {
   public:
    virtual void OnWaybackURLFetched(const GURL& lastest_wayback_url) = 0;
   protected:
    virtual ~Client() = default;
  };

  WaybackMachineURLFetcher(
      Client* client,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~WaybackMachineURLFetcher();

  WaybackMachineURLFetcher(const WaybackMachineURLFetcher&) = delete;
  WaybackMachineURLFetcher& operator=(const WaybackMachineURLFetcher&) = delete;

  void Fetch(const GURL& url);

 private:
  FRIEND_TEST_ALL_PREFIXES(WaybackMachineURLFetcherUnitTest,
                           InputURLSanitizeTest);

  void OnWaybackURLFetched(
      const GURL& original_url,
      api_request_helper::APIRequestResult api_request_result);

  // Clear sensitive data such as username/password from |url|.
  GURL GetSanitizedInputURL(const GURL& url) const;

  // Return empty GURL if |url| is not https/http and its domain is not
  // archive.org.
  GURL GetSanitizedWaybackURL(const GURL& url) const;

  raw_ptr<Client> client_ = nullptr;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> wayback_url_loader_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_MACHINE_URL_FETCHER_H_
