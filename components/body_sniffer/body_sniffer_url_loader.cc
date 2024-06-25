/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/body_sniffer/body_sniffer_url_loader.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/body_sniffer/body_sniffer_throttle.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace body_sniffer {

constexpr const uint32_t kReadBufferSize = 64 * 1024;

// static
std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
           mojo::PendingReceiver<network::mojom::URLLoaderClient>,
           BodySnifferURLLoader*,
           mojo::ScopedDataPipeConsumerHandle>
BodySnifferURLLoader::CreateLoader(
    base::WeakPtr<BodySnifferThrottle> throttle,
    network::mojom::URLResponseHeadPtr response_head,
    Handler handler,
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  mojo::PendingRemote<network::mojom::URLLoader> url_loader;
  mojo::PendingRemote<network::mojom::URLLoaderClient> url_loader_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient>
      url_loader_client_receiver =
          url_loader_client.InitWithNewPipeAndPassReceiver();

  auto loader = base::WrapUnique(new BodySnifferURLLoader(
      std::move(throttle), std::move(response_head), std::move(handler),
      std::move(url_loader_client), std::move(task_runner)));

  mojo::ScopedDataPipeConsumerHandle body_to_send;
  mojo::CreateDataPipe(nullptr, loader->body_producer_handle_, body_to_send);

  BodySnifferURLLoader* loader_rawptr = loader.get();
  mojo::MakeSelfOwnedReceiver(std::move(loader),
                              url_loader.InitWithNewPipeAndPassReceiver());
  return std::make_tuple(std::move(url_loader),
                         std::move(url_loader_client_receiver), loader_rawptr,
                         std::move(body_to_send));
}

BodySnifferURLLoader::BodySnifferURLLoader(
    base::WeakPtr<BodySnifferThrottle> throttle,
    network::mojom::URLResponseHeadPtr response_head,
    Handler handler,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : throttle_(throttle),
      destination_url_loader_client_(std::move(destination_url_loader_client)),
      response_head_(std::move(response_head)),
      handler_(std::move(handler)),
      task_runner_(task_runner),
      body_consumer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             task_runner),
      body_producer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             std::move(task_runner)) {}

BodySnifferURLLoader::~BodySnifferURLLoader() = default;

void BodySnifferURLLoader::Start(
    mojo::PendingRemote<network::mojom::URLLoader> source_url_loader_remote,
    mojo::PendingReceiver<network::mojom::URLLoaderClient>
        source_url_client_receiver,
    mojo::ScopedDataPipeConsumerHandle body) {
  if (!body) {
    return;
  }

  if (auto* producer = absl::get_if<BodyProducerPtr>(&handler_)) {
    buffered_body_ = (*producer)->TakeContent();
  }

  state_ = State::kSniffing;
  if (buffered_body_.empty()) {
    source_url_loader_.Bind(std::move(source_url_loader_remote));
    source_url_client_receiver_.Bind(std::move(source_url_client_receiver),
                                     task_runner_);

    body_consumer_handle_ = std::move(body);
    body_consumer_watcher_.Watch(
        body_consumer_handle_.get(),
        MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
        base::BindRepeating(&BodySnifferURLLoader::OnBodyReadable,
                            base::Unretained(this)));
    body_consumer_watcher_.ArmOrNotify();
  } else {
    complete_status_ = network::URLLoaderCompletionStatus(0);
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&BodySnifferURLLoader::CompleteSniffing,
                                  weak_factory_.GetWeakPtr(), false,
                                  std::move(buffered_body_)));
  }
}

void BodySnifferURLLoader::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {
  // OnReceiveEarlyHints() shouldn't be called. See the comment in
  // OnReceiveResponse().
  NOTREACHED_IN_MIGRATION();
}

void BodySnifferURLLoader::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr response_head,
    mojo::ScopedDataPipeConsumerHandle body,
    std::optional<mojo_base::BigBuffer> cached_metadata) {
  // OnReceiveResponse() shouldn't be called because BodySnifferURLLoader is
  // created by WillProcessResponse(), which is equivalent
  // to OnReceiveResponse().
  NOTREACHED_IN_MIGRATION();
}

void BodySnifferURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr response_head) {
  // OnReceiveRedirect() shouldn't be called because BodySnifferURLLoader is
  // created by WillProcessResponse(), which is equivalent
  // to OnReceiveResponse().
  NOTREACHED_IN_MIGRATION();
}

void BodySnifferURLLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  destination_url_loader_client_->OnUploadProgress(current_position, total_size,
                                                   std::move(ack_callback));
}

void BodySnifferURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  destination_url_loader_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void BodySnifferURLLoader::OnComplete(
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
    case State::kSniffing:
    case State::kSending:
      // Defer calling OnComplete().
      complete_status_ = status;
      return;
    case State::kCompleted:
      destination_url_loader_client_->OnComplete(status);
      return;
    case State::kAborted:
      NOTREACHED_IN_MIGRATION();
      return;
  }
  NOTREACHED_IN_MIGRATION();
}

void BodySnifferURLLoader::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const std::optional<GURL>& new_url) {
  // BodySnifferURLLoader starts handling the request after
  // OnReceivedResponse(). A redirect response is not expected.
  NOTREACHED_IN_MIGRATION();
}

void BodySnifferURLLoader::SetPriority(net::RequestPriority priority,
                                       int32_t intra_priority_value) {
  if (state_ == State::kAborted) {
    return;
  }
  source_url_loader_->SetPriority(priority, intra_priority_value);
}

void BodySnifferURLLoader::PauseReadingBodyFromNet() {
  if (state_ == State::kAborted) {
    return;
  }
  source_url_loader_->PauseReadingBodyFromNet();
}

void BodySnifferURLLoader::ResumeReadingBodyFromNet() {
  if (state_ == State::kAborted) {
    return;
  }
  source_url_loader_->ResumeReadingBodyFromNet();
}

void BodySnifferURLLoader::OnBodyReadable(MojoResult) {
  if (state_ == State::kSending) {
    // The pipe becoming readable when kSending means all buffered body has
    // already been sent.
    ForwardBodyToClient();
    return;
  }
  DCHECK_EQ(State::kSniffing, state_);

  size_t start_size = buffered_body_.size();
  size_t read_bytes = kReadBufferSize;
  buffered_body_.resize(start_size + read_bytes);
  MojoResult result =
      body_consumer_handle_->ReadData(buffered_body_.data() + start_size,
                                      &read_bytes, MOJO_READ_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      buffered_body_.resize(start_size + read_bytes);
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      buffered_body_.resize(start_size);
      break;
    case MOJO_RESULT_SHOULD_WAIT:
      buffered_body_.resize(start_size);
      body_consumer_watcher_.ArmOrNotify();
      return;
    default:
      NOTREACHED_IN_MIGRATION();
      return;
  }

  const bool is_body_complete = result == MOJO_RESULT_FAILED_PRECONDITION;

  auto overall_action = BodyHandler::Action::kNone;

  CHECK(absl::holds_alternative<BodyHandlersPtr>(handler_));
  auto& body_handlers = absl::get<BodyHandlersPtr>(handler_);

  for (auto& handler : body_handlers) {
    const auto action =
        handler->OnBodyUpdated(buffered_body_, is_body_complete);
    switch (action) {
      case BodyHandler::Action::kNone:
        NOTREACHED_IN_MIGRATION() << "Action shouldn't return kNone";
        break;
      case BodyHandler::Action::kContinue:
        if (is_body_complete) {
          overall_action = BodyHandler::Action::kComplete;
        } else {
          overall_action = BodyHandler::Action::kContinue;
        }
        break;
      case BodyHandler::Action::kComplete:
        if (handler->IsTransformer()) {
          CHECK(is_body_complete) << "The transformer cannot be completed "
                                     "until the entire body is received.";
        }
        if (overall_action == BodyHandler::Action::kNone) {
          overall_action = BodyHandler::Action::kComplete;
        }
        break;
      case BodyHandler::Action::kCancel:
        Cancel();
        return;
      case BodyHandler::Action::kAbort:
        Abort();
        return;
    }
  }

  switch (overall_action) {
    case BodyHandler::Action::kNone:
      CompleteSniffing(false, std::move(buffered_body_));
      break;
    case BodyHandler::Action::kCancel:
    case BodyHandler::Action::kAbort:
      break;
    case BodyHandler::Action::kContinue:
      body_consumer_watcher_.ArmOrNotify();
      break;
    case BodyHandler::Action::kComplete:
      for (auto&& handler : body_handlers) {
        if (!handler->IsTransformer()) {
          complete_handlers_.push_back(std::move(handler));
        }
      }
      std::erase_if(body_handlers, [](auto& handler) { return !handler; });
      CompleteSniffing(false, std::move(buffered_body_));
      break;
  }
}

void BodySnifferURLLoader::OnBodyWritable(MojoResult) {
  DCHECK_EQ(State::kSending, state_);
  if (bytes_remaining_in_buffer_ > 0) {
    SendReceivedBodyToClient();
  } else {
    ForwardBodyToClient();
  }
}

void BodySnifferURLLoader::CompleteSniffing(bool remove_first,
                                            std::string body) {
  DCHECK_EQ(State::kSniffing, state_);
  DCHECK(buffered_body_.empty());

  if (auto* body_handlers = absl::get_if<BodyHandlersPtr>(&handler_)) {
    if (remove_first && !body_handlers->empty()) {
      complete_handlers_.push_back(std::move(*body_handlers->begin()));
      body_handlers->erase(body_handlers->begin());
    }

    if (!body_handlers->empty()) {
      auto& transformer = body_handlers->front();
      DCHECK(transformer->IsTransformer());
      transformer->Transform(
          std::move(body),
          base::BindOnce(&BodySnifferURLLoader::CompleteSniffing,
                         weak_factory_.GetWeakPtr(), true));
      return;
    }
    DCHECK(body_handlers->empty());
  }

  state_ = State::kSending;

  buffered_body_ = std::move(body);
  bytes_remaining_in_buffer_ = buffered_body_.size();
  if (!throttle_) {
    Abort();
    return;
  }
  throttle_->Resume();
  // Set up the watcher for the producer handle.
  body_producer_watcher_.Watch(
      body_producer_handle_.get(),
      MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::BindRepeating(&BodySnifferURLLoader::OnBodyWritable,
                          base::Unretained(this)));

  if (bytes_remaining_in_buffer_) {
    SendReceivedBodyToClient();
    return;
  }

  CompleteSending();
}

void BodySnifferURLLoader::CompleteSending() {
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

  if (auto* producer = absl::get_if<BodyProducerPtr>(&handler_)) {
    (*producer)->OnComplete();
  }

  for (auto& handler : complete_handlers_) {
    handler->OnComplete();
  }
  complete_handlers_.clear();
}

void BodySnifferURLLoader::SendReceivedBodyToClient() {
  DCHECK_EQ(State::kSending, state_);
  // Send the buffered data first.
  DCHECK_GT(bytes_remaining_in_buffer_, 0u);
  size_t start_position = buffered_body_.size() - bytes_remaining_in_buffer_;
  size_t bytes_sent = bytes_remaining_in_buffer_;
  MojoResult result =
      body_producer_handle_->WriteData(buffered_body_.data() + start_position,
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
      NOTREACHED_IN_MIGRATION();
      return;
  }
  bytes_remaining_in_buffer_ -= bytes_sent;
  body_producer_watcher_.ArmOrNotify();
}

void BodySnifferURLLoader::ForwardBodyToClient() {
  DCHECK_EQ(0u, bytes_remaining_in_buffer_);
  // Send the body from the consumer to the producer.
  const void* buffer;
  size_t buffer_size = 0;
  MojoResult result =
      body_consumer_handle_
          ? body_consumer_handle_->BeginReadData(&buffer, &buffer_size,
                                                 MOJO_BEGIN_READ_DATA_FLAG_NONE)
          : MOJO_RESULT_FAILED_PRECONDITION;

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
      NOTREACHED_IN_MIGRATION();
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
      NOTREACHED_IN_MIGRATION();
      return;
  }

  body_consumer_handle_->EndReadData(buffer_size);
  body_consumer_watcher_.ArmOrNotify();
}

void BodySnifferURLLoader::Cancel() {
  throttle_->Cancel();
}

void BodySnifferURLLoader::Abort() {
  state_ = State::kAborted;
  body_consumer_watcher_.Cancel();
  body_producer_watcher_.Cancel();
  source_url_loader_.reset();
  source_url_client_receiver_.reset();
  destination_url_loader_client_.reset();
  // |this| should be removed since the owner will destroy |this| or the owner
  // has already been destroyed by some reason.
}

}  // namespace body_sniffer
