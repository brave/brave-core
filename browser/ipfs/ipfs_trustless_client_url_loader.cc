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
#include "base/task/thread_pool.h"
#include "brave/browser/ipfs/ipfs_interrequest_state.h"
#include "brave/components/ipfs/ipld/block_orchestrator.h"
#include "brave/components/ipfs/ipld/trustless_client_types.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace ipfs {

IpfsTrustlessClientUrlLoader::IpfsTrustlessClientUrlLoader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::unique_ptr<ipld::BlockOrchestrator> orchestrator)
    : orchestrator_(std::move(orchestrator)),
      loader_factory_(url_loader_factory) {}

IpfsTrustlessClientUrlLoader::~IpfsTrustlessClientUrlLoader() noexcept {
  LOG(INFO) << "[IPFS] ~IpfsTrustlessClientIrlLoader()";
  //   if (!complete_) {
  //     LOG(ERROR) << "Premature IPFS URLLoader dtor, uri was '" <<
  //     original_url_
  //                << "' " << base::debug::StackTrace();
  //   }
}

void IpfsTrustlessClientUrlLoader::StartRequest(
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

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ipld::BlockOrchestrator::BuildResponse,
          base::Unretained(orchestrator_.get()),
          std::make_unique<ipld::IpfsTrustlessRequest>(original_url_,
                                                       loader_factory_, false),
          base::BindRepeating(
              &IpfsTrustlessClientUrlLoader::OnIpfsTrustlessClientResponse,
              weak_ptr_factory_.GetWeakPtr())));
}

void IpfsTrustlessClientUrlLoader::OnIpfsTrustlessClientResponse(
    std::unique_ptr<ipld::IpfsTrustlessRequest> request,
    std::unique_ptr<ipld::IpfsTrustlessResponse> response) {
  if (!client_) {
    return;
  }

  // LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse "
  //           << std::accumulate(response->body.begin(), response->body.end(),
  //                              std::string(""),
  //                              [](const std::string& a, const uint8_t& b) {
  //                                return a + (char)b;
  //                              });

  PrepareRespponseHead(response->total_size);

  uint32_t write_size = response->body.size();
  MojoResult result = producer_handle_->WriteData(
      &response->body[0], &write_size, MOJO_WRITE_DATA_FLAG_NONE);
  LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse result:" << result;

  if (response->is_last_chunk) {
    client_->OnReceiveResponse(std::move(response_head_),
                               std::move(consumer_handle_), absl::nullopt);
    if (result == MOJO_RESULT_OK) {
      client_->OnComplete(network::URLLoaderCompletionStatus(net::OK));
    } else {
      client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    }
    client_.reset();
  }
}

void IpfsTrustlessClientUrlLoader::PrepareRespponseHead(
    const int64_t& total_size) {
  if (!response_head_) {
    response_head_ = network::mojom::URLResponseHead::New();

    response_head_->request_start = base::TimeTicks::Now();
    response_head_->response_start = base::TimeTicks::Now();
    response_head_->content_length = total_size;
    response_head_->mime_type = "text/plain";

    //    LOG(INFO) << "[IPFS] PrepareRespponseHead total_size:" << total_size;
    if (mojo::CreateDataPipe(total_size, producer_handle_, consumer_handle_) !=
        MOJO_RESULT_OK) {
      //      LOG(INFO) << "[IPFS] PrepareRespponseHead error";
      client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
      client_.reset();
      //  MaybeDeleteSelf();
      return;
    }

    std::string headers("HTTP/1.0 200 OK");
    response_head_->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        net::HttpUtil::AssembleRawHeaders(headers));
    response_head_->headers->AddHeader(
        net::HttpRequestHeaders::kContentLength,
        base::NumberToString(response_head_->content_length));
    if (!response_head_->mime_type.empty()) {
      response_head_->headers->AddHeader(net::HttpRequestHeaders::kContentType,
                                         response_head_->mime_type.c_str());
    }

    // LOG(INFO) << "[IPFS] OnIpfsTrustlessClientResponse is_bound:"
    //           << client_.is_bound() << " is_connected:" <<
    //           client_.is_connected();
  }
}

void IpfsTrustlessClientUrlLoader::FollowRedirect(
    std::vector<std::string> const& removed_headers,
    net::HttpRequestHeaders const& modified_headers,
    net::HttpRequestHeaders const& modified_cors_exempt_headers,
    absl::optional<::GURL> const& new_url) {}

void IpfsTrustlessClientUrlLoader::SetPriority(net::RequestPriority priority,
                                               int32_t intra_priority_value) {}
void IpfsTrustlessClientUrlLoader::PauseReadingBodyFromNet() {}
void IpfsTrustlessClientUrlLoader::ResumeReadingBodyFromNet() {}

}  // namespace ipfs
