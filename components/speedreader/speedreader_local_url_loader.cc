/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_local_url_loader.h"

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "brave/components/speedreader/speedreader_throttle_delegate.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/cpp/record_ontransfersizeupdate_utils.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace speedreader {

// static
std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
           mojo::PendingReceiver<network::mojom::URLLoaderClient>,
           SpeedReaderLocalURLLoader*>
SpeedReaderLocalURLLoader::CreateLoader(//LOADER_EXAMPLE
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
    base::WeakPtr<SpeedreaderThrottleDelegate> delegate,
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  mojo::PendingRemote<network::mojom::URLLoader> url_loader;
  mojo::PendingRemote<network::mojom::URLLoaderClient> url_loader_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient>
      url_loader_client_receiver =
          url_loader_client.InitWithNewPipeAndPassReceiver();

  std::unique_ptr<SpeedReaderLocalURLLoader> loader(
      new SpeedReaderLocalURLLoader(std::move(throttle), std::move(delegate),
                                    std::move(url_loader_client),
                                    std::move(task_runner)));
  SpeedReaderLocalURLLoader* loader_rawptr = loader.get();
  mojo::MakeSelfOwnedReceiver(std::move(loader),
                              url_loader.InitWithNewPipeAndPassReceiver());
  return std::make_tuple(std::move(url_loader),
                         std::move(url_loader_client_receiver), loader_rawptr);
}

SpeedReaderLocalURLLoader::SpeedReaderLocalURLLoader(
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
    base::WeakPtr<SpeedreaderThrottleDelegate> delegate,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : throttle_(std::move(throttle)),
      delegate_(std::move(delegate)),
      destination_url_loader_client_(std::move(destination_url_loader_client)),
      task_runner_(task_runner),
      body_producer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             std::move(task_runner)) {
  mojo::CreateDataPipe(nullptr, body_producer_handle_,
                       destination_consumer_handle_);
}

SpeedReaderLocalURLLoader::~SpeedReaderLocalURLLoader() = default;

mojo::ScopedDataPipeConsumerHandle*
SpeedReaderLocalURLLoader::GetDestinationConsumerHandle() {
  return &destination_consumer_handle_;
}

void SpeedReaderLocalURLLoader::Start(std::string body_content) {
  if (!throttle_) {
    Abort();
    return;
  }

  body_content_ = std::move(body_content);
  bytes_remaining_in_body_content_ = body_content_.size();

  // Set up the watcher for the producer handle.
  body_producer_watcher_.Watch(
      body_producer_handle_.get(),
      MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::BindRepeating(&SpeedReaderLocalURLLoader::OnBodyWritable,
                          base::Unretained(this)));

  if (bytes_remaining_in_body_content_) {
    SendBodyToClient();
    return;
  }

  CompleteSending();
}

void SpeedReaderLocalURLLoader::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {}

void SpeedReaderLocalURLLoader::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr response_head,
    mojo::ScopedDataPipeConsumerHandle body,
    std::optional<mojo_base::BigBuffer> cached_metadata) {}

void SpeedReaderLocalURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr response_head) {}

void SpeedReaderLocalURLLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  destination_url_loader_client_->OnUploadProgress(current_position, total_size,
                                                   std::move(ack_callback));
}

void SpeedReaderLocalURLLoader::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  destination_url_loader_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void SpeedReaderLocalURLLoader::OnComplete(
    const network::URLLoaderCompletionStatus& status) {}

void SpeedReaderLocalURLLoader::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const std::optional<GURL>& new_url) {}

void SpeedReaderLocalURLLoader::SetPriority(net::RequestPriority priority,
                                            int32_t intra_priority_value) {}

void SpeedReaderLocalURLLoader::PauseReadingBodyFromNet() {}

void SpeedReaderLocalURLLoader::ResumeReadingBodyFromNet() {}

void SpeedReaderLocalURLLoader::OnBodyWritable(MojoResult) {
  if (bytes_remaining_in_body_content_ > 0) {
    SendBodyToClient();
  } else {
    CompleteSending();
  }
}

void SpeedReaderLocalURLLoader::CompleteSending() {
  destination_url_loader_client_->OnComplete(
      network::URLLoaderCompletionStatus(0));

  body_producer_watcher_.Cancel();
  body_producer_handle_.reset();

  if (delegate_) {
    delegate_->OnDistillComplete(DistillationResult::kSuccess);
  }
}

void SpeedReaderLocalURLLoader::SendBodyToClient() {
  DCHECK_GT(bytes_remaining_in_body_content_, 0u);

  size_t start_position =
      body_content_.size() - bytes_remaining_in_body_content_;
  uint32_t bytes_sent = bytes_remaining_in_body_content_;
  MojoResult result =
      body_producer_handle_->WriteData(body_content_.data() + start_position,
                                       &bytes_sent, MOJO_WRITE_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // The pipe is closed unexpectedly. |this| should be deleted once
      // URLLoader on the destination is released.
      Abort();
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      body_producer_watcher_.ArmOrNotify();
      return;
    default:
      NOTREACHED();
      return;
  }
  bytes_remaining_in_body_content_ -= bytes_sent;
  body_producer_watcher_.ArmOrNotify();
}

void SpeedReaderLocalURLLoader::Abort() {
  body_producer_watcher_.Cancel();
  destination_url_loader_client_.reset();
  // |this| should be removed since the owner will destroy |this| or the owner
  // has already been destroyed by some reason.
}

}  // namespace speedreader
