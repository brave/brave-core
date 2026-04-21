/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CORE_IPFS_BUNDLE_FETCHER_H_
#define BRAVE_COMPONENTS_WEBCAT_CORE_IPFS_BUNDLE_FETCHER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/webcat/core/bundle_parser.h"
#include "brave/components/webcat/core/constants.h"
#include "url/gurl.h"

namespace webcat {

struct FetchResult {
  bool success = false;
  WebcatError error = WebcatError::kNone;
  std::string error_detail;
  std::optional<Bundle> bundle;
};

class IpfsBundleFetcher {
 public:
  using FetchCallback = base::OnceCallback<void(FetchResult result)>;

  IpfsBundleFetcher(api_request_helper::APIRequestHelper* api_request_helper,
                    const std::string& gateway_host = std::string(kDefaultIpfsGatewayHost),
                    base::TimeDelta timeout = base::Seconds(kDefaultIpfsFetchTimeoutSeconds));
  ~IpfsBundleFetcher();

  IpfsBundleFetcher(const IpfsBundleFetcher&) = delete;
  IpfsBundleFetcher& operator=(const IpfsBundleFetcher&) = delete;

  void FetchBundle(const std::string& cid, FetchCallback callback);

 private:
  void OnFetchResponse(FetchCallback callback,
                       const std::string& cid,
                       api_request_helper::APIRequestResult api_request_result);

  GURL BuildGatewayUrl(const std::string& cid) const;

  raw_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  std::string gateway_host_;
  base::TimeDelta timeout_;
};

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CORE_IPFS_BUNDLE_FETCHER_H_
