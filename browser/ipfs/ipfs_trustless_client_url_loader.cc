/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_trustless_client_url_loader.h"
#include "brave/browser/ipfs/ipfs_interrequest_state.h"

namespace ipfs {

IpfsTrustlessClientIrlLoader::IpfsTrustlessClientIrlLoader(
    network::mojom::URLLoaderFactory& handles_http,
    InterRequestState& state)
    : state_(state),
    loader_factory_(handles_http)//, api_{state_->api()}
{}
IpfsTrustlessClientIrlLoader::~IpfsTrustlessClientIrlLoader() noexcept {
  //   if (!complete_) {
  //     LOG(ERROR) << "Premature IPFS URLLoader dtor, uri was '" <<
  //     original_url_
  //                << "' " << base::debug::StackTrace();
  //   }
}

void IpfsTrustlessClientIrlLoader::StartRequest(
    network::ResourceRequest const& resource_request,
    mojo::PendingReceiver<network::mojom::URLLoader> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
  DCHECK(!receiver_.is_bound());
  DCHECK(!client_.is_bound());
  receiver_.Bind(std::move(receiver));
  client_.Bind(std::move(client));
  if (original_url_.is_empty()) {
    original_url_ = resource_request.url;
  }
  
  LOG(INFO) << "[IPFS] StartRequest: " << original_url_;
}

void IpfsTrustlessClientIrlLoader::FollowRedirect(
    std::vector<std::string> const& removed_headers,
    net::HttpRequestHeaders const& modified_headers,
    net::HttpRequestHeaders const& modified_cors_exempt_headers,
    absl::optional<::GURL> const& new_url) {}

void IpfsTrustlessClientIrlLoader::SetPriority(net::RequestPriority priority,
                                               int32_t intra_priority_value) {}
void IpfsTrustlessClientIrlLoader::PauseReadingBodyFromNet() {}
void IpfsTrustlessClientIrlLoader::ResumeReadingBodyFromNet() {}

}  // namespace ipfs
