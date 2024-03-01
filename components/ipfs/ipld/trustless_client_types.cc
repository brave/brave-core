/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/trustless_client_types.h"

namespace ipfs::ipld {

IpfsTrustlessRequest::IpfsTrustlessRequest(
    const GURL& url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url(url), url_loader_factory(std::move(url_loader_factory)) {}
IpfsTrustlessRequest::~IpfsTrustlessRequest() = default;

IpfsTrustlessResponse::IpfsTrustlessResponse(const std::string& mime,
                           const std::uint16_t& status,
                           const std::vector<uint8_t>& body,
                           const std::string& location)
    : mime(mime), status(status), body(body), location(location) {}
IpfsTrustlessResponse::~IpfsTrustlessResponse() = default;

}  // namespace ipfs::ipld
