/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_TRUSTLESS_CLIENT_TYPES_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_TRUSTLESS_CLIENT_TYPES_H_

#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ipfs::ipld {

struct IpfsRequest {
  IpfsRequest(const IpfsRequest&) = delete;
  IpfsRequest(IpfsRequest&&) = delete;
  IpfsRequest& operator=(const IpfsRequest&) = delete;
  IpfsRequest& operator=(IpfsRequest&&) = delete;

  IpfsRequest(
      GURL url,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~IpfsRequest();

  GURL url;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory;
};

struct IpfsResponse {
  IpfsResponse(const IpfsResponse&) = delete;
  IpfsResponse(IpfsResponse&&) = delete;
  IpfsResponse& operator=(const IpfsResponse&) = delete;
  IpfsResponse& operator=(IpfsResponse&&) = delete;

  IpfsResponse(const std::string& mime,
               const std::uint16_t& status,
               const std::string& body,
               const std::string& location);
  ~IpfsResponse();

  std::string mime;
  std::uint16_t status;
  std::string body;
  std::string location;
};

using IpfsRequestCallback =
    base::OnceCallback<void(std::unique_ptr<IpfsResponse>)>;

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_TRUSTLESS_CLIENT_TYPES_H_
