/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/ipfs_redirect_network_delegate_helper.h"

#include "brave/components/ipfs/translate_ipfs_uri.h"
#include "net/base/net_errors.h"

namespace ipfs {

int OnBeforeURLRequest_IPFSRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  GURL new_url;
  if (ipfs::TranslateIPFSURI(ctx->request_url, &new_url,
                             ctx->ipfs_gateway_url)) {
    ctx->new_url_spec = new_url.spec();
  }
  return net::OK;
}

}  // namespace ipfs
