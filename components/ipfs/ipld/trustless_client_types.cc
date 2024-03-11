/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/trustless_client_types.h"

namespace ipfs::ipld {

IpfsTrustlessRequest::IpfsTrustlessRequest(
    const GURL& url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    bool load_only_structure)
    : url(url),
      url_loader_factory(std::move(url_loader_factory)),
      only_structure(load_only_structure) {}
IpfsTrustlessRequest::~IpfsTrustlessRequest() = default;

IpfsTrustlessResponse::IpfsTrustlessResponse(const std::string& mime,
                                             const std::uint16_t& status,
                                             const std::vector<uint8_t>* body_ptr,
                                             const std::string& location,
                                             const uint64_t& size,
                                             const bool last_chunk)
    : mime(mime),
      status(status),
      body(body_ptr),
      location(location),
      total_size(size),
      is_last_chunk(last_chunk) {}
IpfsTrustlessResponse::~IpfsTrustlessResponse() = default;

DjData::DjData(DjDataType type,
               std::vector<uint8_t> data,
               uint64_t filesize,
               std::vector<uint64_t> blocksizes,
               uint64_t hash_type,
               uint64_t fanout,
               uint64_t mode)
    : type(type),
      data(std::move(data)),
      filesize(filesize),
      blocksizes(std::move(blocksizes)),
      hash_type(hash_type),
      fanout(fanout),
      mode(mode) {}
DjData::~DjData() = default;

}  // namespace ipfs::ipld
