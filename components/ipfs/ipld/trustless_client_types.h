/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_TRUSTLESS_CLIENT_TYPES_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_TRUSTLESS_CLIENT_TYPES_H_

#include <cstdint>
#include "base/functional/callback_forward.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ipfs::ipld {

struct IpfsTrustlessRequest {
  IpfsTrustlessRequest(const IpfsTrustlessRequest&) = delete;
  IpfsTrustlessRequest(IpfsTrustlessRequest&&) = delete;
  IpfsTrustlessRequest& operator=(const IpfsTrustlessRequest&) = delete;
  IpfsTrustlessRequest& operator=(IpfsTrustlessRequest&&) = delete;

  IpfsTrustlessRequest(
      const GURL& url,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      bool load_only_structure = true);
  ~IpfsTrustlessRequest();

  GURL url;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory;
  bool only_structure;
};

struct IpfsTrustlessResponse {
  IpfsTrustlessResponse(const IpfsTrustlessResponse&) = delete;
  IpfsTrustlessResponse(IpfsTrustlessResponse&&) = delete;
  IpfsTrustlessResponse& operator=(const IpfsTrustlessResponse&) = delete;
  IpfsTrustlessResponse& operator=(IpfsTrustlessResponse&&) = delete;

  IpfsTrustlessResponse(const std::string& mime,
                        const std::uint16_t& status,
                        const std::vector<uint8_t>& body,
                        const std::string& location,
                        const uint64_t& size,
                        bool last_chunk);
  ~IpfsTrustlessResponse();

  std::string mime;
  std::uint16_t status;
  std::vector<uint8_t> body;
  std::string location;
  uint64_t total_size;
  bool is_last_chunk;
};

using IpfsRequestCallback =
    base::RepeatingCallback<void(std::unique_ptr<IpfsTrustlessRequest>,
                                 std::unique_ptr<IpfsTrustlessResponse>)>;

struct TrustlessTarget {
  std::string cid;
  std::string path;

  bool IsCidTarget() const { return !cid.empty() && path.empty(); }

  bool IsPathTarget() const { return !path.empty(); }
};

struct StringHash {
  using is_transparent = void;
  std::size_t operator()(std::string_view sv) const {
    std::hash<std::string_view> hasher;
    return hasher(sv);
  }
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_TRUSTLESS_CLIENT_TYPES_H_
