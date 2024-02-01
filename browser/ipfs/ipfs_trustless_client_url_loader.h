/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "partition_alloc/pointers/raw_ptr.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace network::mojom {
class URLLoaderFactory;
}  // namespace network::mojom

namespace ipfs {
class InterRequestState;

class IpfsTrustlessClientIrlLoader : public network::mojom::URLLoader {

public:
  explicit IpfsTrustlessClientIrlLoader(network::mojom::URLLoaderFactory& handles_http,
                         InterRequestState& state);
  ~IpfsTrustlessClientIrlLoader() override;

void StartRequest(
      network::ResourceRequest const& resource_request,
      mojo::PendingReceiver<network::mojom::URLLoader> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client);

private:
  void FollowRedirect(
      std::vector<std::string> const& removed_headers,
      net::HttpRequestHeaders const& modified_headers,
      net::HttpRequestHeaders const& modified_cors_exempt_headers,
      absl::optional<::GURL> const& new_url) override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;


raw_ref<InterRequestState> state_;
raw_ref<network::mojom::URLLoaderFactory> loader_factory_;
mojo::Receiver<network::mojom::URLLoader> receiver_{this};
mojo::Remote<network::mojom::URLLoaderClient> client_;

GURL original_url_;
};

}  // namespace ipfs {    