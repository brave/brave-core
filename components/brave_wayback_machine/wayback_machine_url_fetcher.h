/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_MACHINE_URL_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_MACHINE_URL_FETCHER_H_

#include <memory>
#include <string_view>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
}  // network

class GURL;

// Looks up the latest Wayback Machine snapshot of a URL. At most one lookup is
// in flight at a time: starting a new lookup or calling Cancel() drops the
// pending one, and the client is never notified about a dropped lookup.
// Destroying the fetcher also drops the pending lookup.
class WaybackMachineURLFetcher final {
 public:
  // Receives the results of lookups started with Fetch().
  class Client {
   public:
    // Called asynchronously once for each lookup that is not dropped.
    // |latest_wayback_url| is an https URL on the Wayback Machine host, or
    // empty if no snapshot is available or the response is invalid.
    // |snapshot_time| is null if |latest_wayback_url| is empty or the time of
    // the snapshot is unknown.
    virtual void OnWaybackURLFetched(const GURL& latest_wayback_url,
                                     base::Time snapshot_time) = 0;

   protected:
    virtual ~Client() = default;
  };

  // |client| must outlive the fetcher.
  WaybackMachineURLFetcher(
      Client* client,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~WaybackMachineURLFetcher();

  WaybackMachineURLFetcher(const WaybackMachineURLFetcher&) = delete;
  WaybackMachineURLFetcher& operator=(const WaybackMachineURLFetcher&) = delete;

  // Starts looking up the latest snapshot of |url|, dropping any pending
  // lookup. The fragment and credentials of |url| are not sent.
  void Fetch(const GURL& url);

  // Drops the pending lookup, if any.
  void Cancel();

 private:
  FRIEND_TEST_ALL_PREFIXES(WaybackMachineURLFetcherUnitTest,
                           InputURLSanitizeTest);
  FRIEND_TEST_ALL_PREFIXES(WaybackMachineURLFetcherUnitTest,
                           ParseSnapshotTimestampTest);

  void OnWaybackURLFetched(
      api_request_helper::APIRequestResult api_request_result);

  // Clear sensitive data such as username/password from |url|.
  GURL GetSanitizedInputURL(const GURL& url) const;

  // Return empty GURL if |url| is not https/http and its domain is not
  // archive.org.
  GURL GetSanitizedWaybackURL(const GURL& url) const;

  // Parses a UTC "YYYYMMDDhhmmss" timestamp. Returns a null time on failure.
  static base::Time ParseSnapshotTimestamp(std::string_view timestamp);

  raw_ptr<Client> client_ = nullptr;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_MACHINE_URL_FETCHER_H_
