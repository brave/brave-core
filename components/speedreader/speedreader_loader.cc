/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_loader.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/post_task.h"
#include "brave/components/speedreader/speedreader_throttle.h"
#include "components/grit/brave_components_resources.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "ui/base/resource/resource_bundle.h"
#include "brave/components/speedreader/rust/ffi/include/speedreader.hpp"

namespace speedreader {

namespace {

constexpr uint32_t kReadBufferSize = 32768;

std::string GetDistilledPageResources() {
  return "<style id=\"brave_speedreader_style\">" +
         ui::ResourceBundle::GetSharedInstance()
             .GetRawDataResource(IDR_SPEEDREADER_STYLE_DESKTOP)
             .as_string() +
         "</style>";
}

}  // namespace

// static
std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
           mojo::PendingReceiver<network::mojom::URLLoaderClient>,
           SpeedReaderURLLoader*>
SpeedReaderURLLoader::CreateLoader(
    base::WeakPtr<SpeedReaderThrottle> throttle,
    const GURL& response_url,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  mojo::PendingRemote<network::mojom::URLLoader> url_loader;
  mojo::PendingRemote<network::mojom::URLLoaderClient> url_loader_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient>
      url_loader_client_receiver =
          url_loader_client.InitWithNewPipeAndPassReceiver();

  auto loader = base::WrapUnique(new SpeedReaderURLLoader(
      std::move(throttle), response_url, std::move(url_loader_client),
      std::move(task_runner)));
  SpeedReaderURLLoader* loader_rawptr = loader.get();
  mojo::MakeSelfOwnedReceiver(std::move(loader),
                              url_loader.InitWithNewPipeAndPassReceiver());
  return std::make_tuple(std::move(url_loader),
                         std::move(url_loader_client_receiver), loader_rawptr);
}

SpeedReaderURLLoader::SpeedReaderURLLoader(
    base::WeakPtr<SpeedReaderThrottle> throttle,
    const GURL& response_url,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : throttle_(throttle),
      destination_url_loader_client_(std::move(destination_url_loader_client)),
      response_url_(response_url),
      task_runner_(task_runner),
      body_consumer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             task_runner),
      body_producer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             std::move(task_runner)) {}

SpeedReaderURLLoader::~SpeedReaderURLLoader() = default;

void SpeedReaderURLLoader::Start(
    mojo::PendingRemote<network::mojom::URLLoader> source_url_loader_remote,
    mojo::PendingReceiver<network::mojom::URLLoaderClient>
        source_url_client_receiver) {
  source_url_loader_.Bind(std::move(source_url_loader_remote));
  source_url_client_receiver_.Bind(std::move(source_url_client_receiver),
                                   task_runner_);
}

void SpeedReaderURLLoader::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr response_head) {
  // OnReceiveResponse() shouldn't be called because SpeedReaderURLLoader is
  // created by SpeedReaderThrottle::WillProcessResponse(), which is equivalent
  // to OnReceiveResponse().
  NOTREACHED();
}

void SpeedReaderURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr response_head) {
  // OnReceiveRedirect() shouldn't be called because SpeedReaderURLLoader is
  // created by SpeedReaderThrottle::WillProcessResponse(), which is equivalent
  // to OnReceiveResponse().
  NOTREACHED();
}

void SpeedReaderURLLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  destination_url_loader_client_->OnUploadProgress(current_position, total_size,
                                                   std::move(ack_callback));
}

void SpeedReaderURLLoader::OnReceiveCachedMetadata(mojo_base::BigBuffer data) {
  destination_url_loader_client_->OnReceiveCachedMetadata(std::move(data));
}

void SpeedReaderURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  destination_url_loader_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void SpeedReaderURLLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  VLOG(2) << __func__ << " " << response_url_;
  state_ = State::kLoading;
  body_consumer_handle_ = std::move(body);
  body_consumer_watcher_.Watch(
      body_consumer_handle_.get(),
      MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::BindRepeating(&SpeedReaderURLLoader::OnBodyReadable,
                          base::Unretained(this)));
  body_consumer_watcher_.ArmOrNotify();
}

void SpeedReaderURLLoader::OnComplete(
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
      // Defer calling OnComplete() until distilling has finished and all
      // data is sent.
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

void SpeedReaderURLLoader::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const base::Optional<GURL>& new_url) {
  // SpeedReaderURLLoader starts handling the request after
  // OnReceivedResponse(). A redirect response is not expected.
  NOTREACHED();
}

void SpeedReaderURLLoader::SetPriority(net::RequestPriority priority,
                                       int32_t intra_priority_value) {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->SetPriority(priority, intra_priority_value);
}

void SpeedReaderURLLoader::PauseReadingBodyFromNet() {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->PauseReadingBodyFromNet();
}

void SpeedReaderURLLoader::ResumeReadingBodyFromNet() {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->ResumeReadingBodyFromNet();
}

void SpeedReaderURLLoader::OnBodyReadable(MojoResult) {
  DCHECK_EQ(State::kLoading, state_);

  size_t start_size = buffered_body_.size();
  uint32_t read_bytes = kReadBufferSize;
  buffered_body_.resize(start_size + read_bytes);
  MojoResult result = body_consumer_handle_->ReadData(
      &buffered_body_[0] + start_size, &read_bytes, MOJO_READ_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // Reading is finished.
      buffered_body_.resize(start_size);
      MaybeLaunchSpeedreader();
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      body_consumer_watcher_.ArmOrNotify();
      return;
    default:
      NOTREACHED();
      return;
  }

  DCHECK_EQ(MOJO_RESULT_OK, result);
  buffered_body_.resize(start_size + read_bytes);
  // TODO(iefremov): We actually can partially |pumpContent| to speedreader,
  // but skipping it for now to simplify things. Pumping is not free in terms
  // of CPU ticks, so we will have to keep alive speedreader instance on another
  // thread.

  body_consumer_watcher_.ArmOrNotify();
}

void SpeedReaderURLLoader::OnBodyWritable(MojoResult r) {
  DCHECK_EQ(State::kSending, state_);
  if (bytes_remaining_in_buffer_ > 0) {
    SendReceivedBodyToClient();
  } else {
    CompleteSending();
  }
}

void SpeedReaderURLLoader::MaybeLaunchSpeedreader() {
  DCHECK_EQ(State::kLoading, state_);
  if (!throttle_) {
    Abort();
    return;
  }

  VLOG(2) << __func__ << " buffered body size = " << buffered_body_.size();
  bytes_remaining_in_buffer_ = buffered_body_.size();

  if (bytes_remaining_in_buffer_ > 0) {
    // Offload heavy distilling to another thread.
    base::PostTaskAndReplyWithResult(
        FROM_HERE, {base::ThreadPool(), base::TaskPriority::USER_BLOCKING},
        base::BindOnce(
            [](GURL url, std::string data) -> auto {
              SCOPED_UMA_HISTOGRAM_TIMER("Brave.Speedreader.Distill");
              SpeedReader speedreader;
              if (!speedreader.ReadableURL(url.spec())) {
                return std::string();
              }
              auto rewriter = speedreader.RewriterNew(url.spec());
              int written = rewriter->Write(data.c_str(), data.length());
              // Error occurred
              if (written != 0) {
                return std::string();
              }

              rewriter->End();
              std::string transformed = *rewriter->GetOutput();

              return GetDistilledPageResources() + transformed;
            },
            response_url_, std::move(buffered_body_)),
        base::BindOnce(&SpeedReaderURLLoader::CompleteLoading,
                       weak_factory_.GetWeakPtr()));
    return;
  }
  CompleteLoading(std::move(buffered_body_));
}

void SpeedReaderURLLoader::CompleteLoading(std::string body) {
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
      mojo::CreateDataPipe(nullptr, &body_producer_handle_, &body_to_send);
  if (result != MOJO_RESULT_OK) {
    Abort();
    return;
  }
  // Set up the watcher for the producer handle.
  body_producer_watcher_.Watch(
      body_producer_handle_.get(),
      MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::BindRepeating(&SpeedReaderURLLoader::OnBodyWritable,
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

void SpeedReaderURLLoader::CompleteSending() {
  DCHECK_EQ(State::kSending, state_);
  state_ = State::kCompleted;
  // Call client's OnComplete() if |this|'s OnComplete() has already been
  // called.
  if (complete_status_.has_value())
    destination_url_loader_client_->OnComplete(complete_status_.value());

  body_consumer_watcher_.Cancel();
  body_producer_watcher_.Cancel();
  body_consumer_handle_.reset();
  body_producer_handle_.reset();
}

void SpeedReaderURLLoader::SendReceivedBodyToClient() {
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

void SpeedReaderURLLoader::Abort() {
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

}  // namespace speedreader
