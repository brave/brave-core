/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include "brave/components/ipfs/ipld/block_orchestrator.h"
#include "brave/components/ipfs/ipld/trustless_client_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "partition_alloc/pointers/raw_ptr.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/gurl.h"

namespace network::mojom {
class URLLoaderFactory;
}  // namespace network::mojom

namespace ipfs {
namespace ipld {
class BlockOrchestrator;
}

class IpfsTrustlessClientUrlLoader : public network::mojom::URLLoader {
 public:
  explicit IpfsTrustlessClientUrlLoader(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<ipld::BlockOrchestrator> orchestrator);
  ~IpfsTrustlessClientUrlLoader() override;

  void StartRequest(
      network::ResourceRequest const& resource_request,
      mojo::PendingReceiver<network::mojom::URLLoader> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client);

 private:
  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const ::net::HttpRequestHeaders& modified_headers,
      const ::net::HttpRequestHeaders& modified_cors_exempt_headers,
      const std::optional<::GURL>& new_url) override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  void PrepareRespponseHead(const int64_t& total_size);

  void OnIpfsTrustlessClientResponse(
      std::unique_ptr<ipld::IpfsTrustlessRequest> request,
      std::unique_ptr<ipld::IpfsTrustlessResponse> response);

  std::unique_ptr<ipld::BlockOrchestrator> orchestrator_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
  mojo::Receiver<network::mojom::URLLoader> receiver_{this};
  mojo::Remote<network::mojom::URLLoaderClient> client_;
  network::mojom::URLResponseHeadPtr response_head_;
  mojo::ScopedDataPipeProducerHandle producer_handle_;
  mojo::ScopedDataPipeConsumerHandle consumer_handle_;

  GURL original_url_;
  base::WeakPtrFactory<IpfsTrustlessClientUrlLoader> weak_ptr_factory_{this};
};

}  // namespace ipfs
