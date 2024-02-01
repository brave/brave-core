/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */


#ifndef BRAVE_COMPONENTS_IPFS_IPFS_TRUSTLESS_URL_LOADER_THROTTLE_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_TRUSTLESS_URL_LOADER_THROTTLE_H_

#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace ipfs {
//  //IN_DEVELOPMENT
// class IpfsTrustlessUrlLoaderThrottle : public blink::URLLoaderThrottle {
//  public:
//   explicit IpfsTrustlessUrlLoaderThrottle();
//   ~IpfsTrustlessUrlLoaderThrottle() override;

//   void WillStartRequest(network::ResourceRequest* request,
//                         bool* defer) override;

//   void WillRedirectRequest(
//       net::RedirectInfo* redirect_info,
//       const network::mojom::URLResponseHead& response_head,
//       bool* defer,
//       std::vector<std::string>* to_be_removed_headers,
//       net::HttpRequestHeaders* modified_headers,
//       net::HttpRequestHeaders* modified_cors_exempt_headers) override;
// };

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_TRUSTLESS_URL_LOADER_THROTTLE_H_