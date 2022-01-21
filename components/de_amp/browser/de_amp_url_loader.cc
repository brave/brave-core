/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_url_loader.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/de_amp/browser/de_amp_service.h"
#include "brave/components/de_amp/browser/de_amp_throttle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace de_amp {

namespace {

constexpr uint32_t kReadBufferSize = 65536;

}  // namespace

// static
std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
           mojo::PendingReceiver<network::mojom::URLLoaderClient>,
           DeAmpURLLoader*>
DeAmpURLLoader::CreateLoader(
    base::WeakPtr<DeAmpThrottle> throttle,
    const GURL& response_url,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    DeAmpService* service,
    content::WebContents* contents) {
  mojo::PendingRemote<network::mojom::URLLoader> url_loader;
  mojo::PendingRemote<network::mojom::URLLoaderClient> url_loader_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient>
      url_loader_client_receiver =
          url_loader_client.InitWithNewPipeAndPassReceiver();

  auto loader = base::WrapUnique(new DeAmpURLLoader(
      std::move(throttle), response_url, std::move(url_loader_client),
      std::move(task_runner), service, contents));
  DeAmpURLLoader* loader_rawptr = loader.get();
  mojo::MakeSelfOwnedReceiver(std::move(loader),
                              url_loader.InitWithNewPipeAndPassReceiver());
  return std::make_tuple(std::move(url_loader),
                         std::move(url_loader_client_receiver), loader_rawptr);
}

DeAmpURLLoader::DeAmpURLLoader(
    base::WeakPtr<DeAmpThrottle> throttle,
    const GURL& response_url,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    DeAmpService* service,
    content::WebContents* contents)
    : throttle_(throttle),
      destination_url_loader_client_(std::move(destination_url_loader_client)),
      response_url_(response_url),
      contents_(contents),
      de_amp_service(service),
      task_runner_(task_runner),
      body_consumer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             task_runner),
      body_producer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             std::move(task_runner)) {}

DeAmpURLLoader::~DeAmpURLLoader() = default;

void DeAmpURLLoader::Start(
    mojo::PendingRemote<network::mojom::URLLoader> source_url_loader_remote,
    mojo::PendingReceiver<network::mojom::URLLoaderClient>
        source_url_client_receiver) {
  source_url_loader_.Bind(std::move(source_url_loader_remote));
  source_url_client_receiver_.Bind(std::move(source_url_client_receiver),
                                   task_runner_);
}

void DeAmpURLLoader::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {}

void DeAmpURLLoader::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr response_head) {
  // OnReceiveResponse() shouldn't be called because DeAmpURLLoader is
  // created by DeAmpThrottle::WillProcessResponse(), which is equivalent
  // to OnReceiveResponse().
  NOTREACHED();
}

void DeAmpURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr response_head) {
  // OnReceiveRedirect() shouldn't be called because DeAmpURLLoader is
  // created by DeAmpThrottle::WillProcessResponse(), which is equivalent
  // to OnReceiveResponse().
  NOTREACHED();
}

void DeAmpURLLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  destination_url_loader_client_->OnUploadProgress(current_position, total_size,
                                                   std::move(ack_callback));
}

void DeAmpURLLoader::OnReceiveCachedMetadata(mojo_base::BigBuffer data) {
  destination_url_loader_client_->OnReceiveCachedMetadata(std::move(data));
}

void DeAmpURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  destination_url_loader_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void DeAmpURLLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  VLOG(2) << __func__ << " " << response_url_;
  state_ = State::kLoading;
  body_consumer_handle_ = std::move(body);
  body_consumer_watcher_.Watch(
      body_consumer_handle_.get(),
      MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::BindRepeating(&DeAmpURLLoader::OnBodyReadable,
                          base::Unretained(this)));
  body_consumer_watcher_.ArmOrNotify();
}

void DeAmpURLLoader::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  DCHECK(!complete_status_.has_value());
  switch (state_) {
    case State::kWaitForBody:
      // An error occured before receiving any data.
      DCHECK_NE(net::OK, status.error_code);
      state_ = State::kCompleted;
      if (!throttle_) {
        Abort();
        return;
      }
      throttle_->Resume();
      destination_url_loader_client_->OnComplete(status);
      return;
    case State::kLoading:
    case State::kSending:
      complete_status_ = status;
      return;
    case State::kCompleted:
      destination_url_loader_client_->OnComplete(status);
      return;
    case State::kAborted:
      NOTREACHED();
      return;
  }
  NOTREACHED();
}

void DeAmpURLLoader::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const absl::optional<GURL>& new_url) {
  // DeAmpURLLoader starts handling the request after
  // OnReceivedResponse(). A redirect response is not expected.
  NOTREACHED();
}

void DeAmpURLLoader::SetPriority(net::RequestPriority priority,
                                       int32_t intra_priority_value) {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->SetPriority(priority, intra_priority_value);
}

void DeAmpURLLoader::PauseReadingBodyFromNet() {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->PauseReadingBodyFromNet();
}

void DeAmpURLLoader::ResumeReadingBodyFromNet() {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->ResumeReadingBodyFromNet();
}

void DeAmpURLLoader::OnBodyReadable(MojoResult) {
  if (state_ == State::kSending) {
    // The pipe becoming readable when kSending means all buffered body has
    // already been sent.
    ForwardBodyToClient();
    return;
  }

  size_t start_size = buffered_body_.size();
  uint32_t read_bytes = kReadBufferSize;
  buffered_body_.resize(start_size + read_bytes);
  auto result = body_consumer_handle_->ReadData(
      &buffered_body_[0] + start_size, &read_bytes, MOJO_READ_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // Reading is finished.
      buffered_body_.resize(start_size);
      CompleteLoading(std::move(buffered_body_));
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      body_consumer_watcher_.ArmOrNotify();
      return;
    default:
      NOTREACHED();
      return;
  }
  DCHECK_EQ(MOJO_RESULT_OK, result);

  // Check for AMP-ness and find canonical link
  std::string canonical_link;

  if (de_amp_service->FindCanonicalLinkIfAMP(buffered_body_, &canonical_link)) {
    const GURL canonical_url(canonical_link);
    if (!de_amp_service->CheckCanonicalLink(canonical_url)) {
      VLOG(2) << __func__ << " canonical link check failed " << canonical_url;
      CompleteLoading(std::move(buffered_body_));
      return;
    }
    VLOG(2) << __func__ << " de-amping and loading " << canonical_url;
    contents_->GetController().LoadURL(canonical_url, content::Referrer(),
                                       ui::PAGE_TRANSITION_CLIENT_REDIRECT,
                                       std::string());
  } else {
    // Did not find AMP page and/or canonical link, load original
    CompleteLoading(std::move(buffered_body_));
    return;
  }

  body_consumer_watcher_.ArmOrNotify();
}

void DeAmpURLLoader::OnBodyWritable(MojoResult r) {
  DCHECK_EQ(State::kSending, state_);
  if (bytes_remaining_in_buffer_ > 0) {
    SendReceivedBodyToClient();
  } else {
    ForwardBodyToClient();
  }
}

void DeAmpURLLoader::CompleteLoading(std::string body) {
  DCHECK_EQ(State::kLoading, state_);
  state_ = State::kSending;

  if (!throttle_) {
    Abort();
    return;
  }

  buffered_body_ = std::move(body);
  bytes_remaining_in_buffer_ = buffered_body_.size();

  throttle_->Resume();
  mojo::ScopedDataPipeConsumerHandle body_to_send;
  MojoResult result =
      mojo::CreateDataPipe(nullptr, body_producer_handle_, body_to_send);
  if (result != MOJO_RESULT_OK) {
    Abort();
    return;
  }
  // Set up the watcher for the producer handle.
  body_producer_watcher_.Watch(
      body_producer_handle_.get(),
      MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::BindRepeating(&DeAmpURLLoader::OnBodyWritable,
                          base::Unretained(this)));

  // Send deferred message.
  destination_url_loader_client_->OnStartLoadingResponseBody(
      std::move(body_to_send));

  DCHECK(bytes_remaining_in_buffer_);
  if (bytes_remaining_in_buffer_) {
    SendReceivedBodyToClient();
    return;
  }

  CompleteSending();
}

void DeAmpURLLoader::ForwardBodyToClient() {
  DCHECK_EQ(0u, bytes_remaining_in_buffer_);
  // Send the body from the consumer to the producer.
  const void* buffer;
  uint32_t buffer_size = 0;
  MojoResult result = body_consumer_handle_->BeginReadData(
      &buffer, &buffer_size, MOJO_BEGIN_READ_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_SHOULD_WAIT:
      body_consumer_watcher_.ArmOrNotify();
      return;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // All data has been sent.
      CompleteSending();
      return;
    default:
      NOTREACHED();
      return;
  }

  result = body_producer_handle_->WriteData(buffer, &buffer_size,
                                            MOJO_WRITE_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // The pipe is closed unexpectedly. |this| should be deleted once
      // URLLoader on the destination is released.
      Abort();
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      body_consumer_handle_->EndReadData(0);
      body_producer_watcher_.ArmOrNotify();
      return;
    default:
      NOTREACHED();
      return;
  }

  body_consumer_handle_->EndReadData(buffer_size);
  body_consumer_watcher_.ArmOrNotify();
}

void DeAmpURLLoader::CompleteSending() {
  DCHECK_EQ(State::kSending, state_);
  state_ = State::kCompleted;
  // Call client's OnComplete() if |this|'s OnComplete() has already been
  // called.
  if (complete_status_.has_value()) {
    destination_url_loader_client_->OnComplete(complete_status_.value());
  }

  body_consumer_watcher_.Cancel();
  body_producer_watcher_.Cancel();
  body_consumer_handle_.reset();
  body_producer_handle_.reset();
}

void DeAmpURLLoader::SendReceivedBodyToClient() {
  DCHECK_EQ(State::kSending, state_);
  // Send the buffered data first.
  DCHECK_GT(bytes_remaining_in_buffer_, 0u);
  size_t start_position = buffered_body_.size() - bytes_remaining_in_buffer_;
  uint32_t bytes_sent = bytes_remaining_in_buffer_;
  MojoResult result =
      body_producer_handle_->WriteData(buffered_body_.data() + start_position,
                                       &bytes_sent, MOJO_WRITE_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // The pipe is closed unexpectedly. |this| should be deleted once
      // URLLoaderPtr on the destination is released.
      Abort();
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      body_producer_watcher_.ArmOrNotify();
      return;
    default:
      NOTREACHED();
      return;
  }
  bytes_remaining_in_buffer_ -= bytes_sent;
  body_producer_watcher_.ArmOrNotify();
}

void DeAmpURLLoader::Abort() {
  VLOG(2) << __func__ << " " << response_url_;
  state_ = State::kAborted;
  body_consumer_watcher_.Cancel();
  body_producer_watcher_.Cancel();
  source_url_loader_.reset();
  source_url_client_receiver_.reset();
  destination_url_loader_client_.reset();
  // |this| should be removed since the owner will destroy |this| or the owner
  // has already been destroyed by some reason.
}

}  // namespace de_amp
