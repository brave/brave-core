/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_trustless_client_url_loader.h"
#include <cstdint>
#include <memory>
#include <utility>
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/task/task_traits.h"
#include "brave/browser/ipfs/ipfs_interrequest_state.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "brave/components/ipfs/ipld/trustless_client_types.h"
#include "brave/components/ipfs/ipld/block_orchestrator_service.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "base/task/thread_pool.h"

namespace ipfs {

IpfsTrustlessClientIrlLoader::IpfsTrustlessClientIrlLoader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    ipld::BlockOrchestratorService& orchestrator)
    : orchestrator_(orchestrator),
      loader_factory_(url_loader_factory) {}

IpfsTrustlessClientIrlLoader::~IpfsTrustlessClientIrlLoader() noexcept {
  LOG(INFO) << "[IPFS] ~IpfsTrustlessClientIrlLoader()";
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

  // auto ptr = loader_factory_->Clone();

  // content::GetUIThreadTaskRunner({})->PostTask(
  //       FROM_HERE, 
  //       base::BindOnce(&IpfsTrustlessClientIrlLoader::RequestOrchestrator,
  //                      base::Owned(this),original_url_, std::move(ptr)));

  orchestrator_->BuildResponse(
      std::make_unique<ipld::IpfsTrustlessRequest>(
          original_url_,
          loader_factory_),
      base::BindRepeating(
          &IpfsTrustlessClientIrlLoader::OnIpfsTrustlessClientResponse,
          weak_ptr_factory_.GetWeakPtr()
          ));

  // Working approach
  // std::string data = "hdfjskahfdjkal";
  // std::vector<uint8_t> data_v(data.begin(), data.end());
  // OnIpfsTrustlessClientResponse(nullptr, std::make_unique<ipld::IpfsTrustlessResponse>("", 200, data_v, ""));

}

void IpfsTrustlessClientIrlLoader::RequestOrchestrator(const GURL& url, std::unique_ptr<network::PendingSharedURLLoaderFactory> pending_factory) {  

if(orchestrator_->IsActive()){
  LOG(INFO) << "[IPFS] RequestOrchestrator orchestrator_->IsActive()";
   return;
}

  auto ptr = network::SharedURLLoaderFactory::Create(std::move(pending_factory));
  orchestrator_->BuildResponse(
      std::make_unique<ipld::IpfsTrustlessRequest>(
          url,
          ptr),
      base::BindRepeating(
          &IpfsTrustlessClientIrlLoader::OnIpfsTrustlessClientResponse,
          weak_ptr_factory_.GetWeakPtr()
          ));
}

void IpfsTrustlessClientIrlLoader::OnIpfsTrustlessClientResponse(
    std::unique_ptr<ipld::IpfsTrustlessRequest> request,
    std::unique_ptr<ipld::IpfsTrustlessResponse> response) {
  if (!client_) {
    return;
  }

  LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse "
            << std::accumulate(response->body.begin(), response->body.end(),
                               std::string(""),
                               [](const std::string& a, const uint8_t& b) {
                                 return a + (char)b;
                               });

  auto head = network::mojom::URLResponseHead::New();
  head->request_start = base::TimeTicks::Now();
  head->response_start = base::TimeTicks::Now();
  head->content_length = response->body.size();
  head->mime_type = "text/plain";
  mojo::ScopedDataPipeProducerHandle producer_handle;
  mojo::ScopedDataPipeConsumerHandle consumer_handle;
LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse #10";
  if (mojo::CreateDataPipe(response->body.size(), producer_handle,
                           consumer_handle) != MOJO_RESULT_OK) {
LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse error";
    client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    client_.reset();
    //  MaybeDeleteSelf();
    return;
  }

std::string headers("HTTP/1.0 200 OK");
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
                  net::HttpUtil::AssembleRawHeaders(headers));
  head->headers->AddHeader(net::HttpRequestHeaders::kContentLength,
                           base::NumberToString(head->content_length));
  if (!head->mime_type.empty()) {
    head->headers->AddHeader(net::HttpRequestHeaders::kContentType,
                             head->mime_type.c_str());
  }
LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse is_bound:" << client_.is_bound() << " is_connected:" << client_.is_connected();

 client_->OnReceiveResponse(std::move(head), std::move(consumer_handle),
                            absl::nullopt);

LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse #20";
  uint32_t write_size = response->body.size();
  MojoResult result = producer_handle->WriteData(&response->body[0], &write_size,
                                                 MOJO_WRITE_DATA_FLAG_NONE);
LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse result:" << result;

    if (result == MOJO_RESULT_OK)
      client_->OnComplete(network::URLLoaderCompletionStatus(net::OK));
    else
      client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    client_.reset();                                                 
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
