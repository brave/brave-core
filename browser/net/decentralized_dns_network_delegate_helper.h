/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_DECENTRALIZED_DNS_NETWORK_DELEGATE_HELPER_H_
#define BRAVE_BROWSER_NET_DECENTRALIZED_DNS_NETWORK_DELEGATE_HELPER_H_

#include <memory>
#include <string>

#include "brave/browser/net/url_context.h"
#include "net/base/completion_once_callback.h"

namespace decentralized_dns {

// Issue eth_call requests via Ethereum provider such as Infura to query
// decentralized DNS records, and redirect URL requests based on them.
int OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx);

void OnBeforeURLRequest_DecentralizedDnsRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    bool success,
    const std::string& result);

void OnBeforeURLRequest_EnsRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    bool success,
    const std::string& ipfs_uri);

}  // namespace decentralized_dns

#endif  // BRAVE_BROWSER_NET_DECENTRALIZED_DNS_NETWORK_DELEGATE_HELPER_H_
