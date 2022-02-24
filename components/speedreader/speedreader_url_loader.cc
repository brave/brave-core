/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_url_loader.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/speedreader/rust/ffi/speedreader.h"
#include "brave/components/speedreader/speedreader_result_delegate.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_throttle.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace speedreader {

namespace {

constexpr uint32_t kReadBufferSize = 32768;

}  // namespace

// static
std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
           mojo::PendingReceiver<network::mojom::URLLoaderClient>,
           SpeedReaderURLLoader*>
SpeedReaderURLLoader::CreateLoader(
    base::WeakPtr<sniffer::SnifferThrottle> throttle,
    base::WeakPtr<SpeedreaderResultDelegate> delegate,
    const GURL& response_url,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    SpeedreaderRewriterService* rewriter_service) {
  mojo::PendingRemote<network::mojom::URLLoader> url_loader;
  mojo::PendingRemote<network::mojom::URLLoaderClient> url_loader_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient>
      url_loader_client_receiver =
          url_loader_client.InitWithNewPipeAndPassReceiver();

  auto loader = base::WrapUnique(new SpeedReaderURLLoader(
      std::move(throttle), std::move(delegate), response_url,
      std::move(url_loader_client), std::move(task_runner), rewriter_service));
  SpeedReaderURLLoader* loader_rawptr = loader.get();
  mojo::MakeSelfOwnedReceiver(std::move(loader),
                              url_loader.InitWithNewPipeAndPassReceiver());
  return std::make_tuple(std::move(url_loader),
                         std::move(url_loader_client_receiver), loader_rawptr);
}

SpeedReaderURLLoader::SpeedReaderURLLoader(
    base::WeakPtr<sniffer::SnifferThrottle> throttle,
    base::WeakPtr<SpeedreaderResultDelegate> delegate,
    const GURL& response_url,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    SpeedreaderRewriterService* rewriter_service)
    : sniffer::SnifferURLLoader(throttle,
                                response_url,
                                std::move(destination_url_loader_client),
                                task_runner),
      delegate_(delegate),
      rewriter_service_(rewriter_service) {}

SpeedReaderURLLoader::~SpeedReaderURLLoader() = default;

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
  if (!throttle_ || !rewriter_service_) {
    Abort();
    return;
  }

  VLOG(2) << __func__ << " buffered body size = " << buffered_body_.size();
  bytes_remaining_in_buffer_ = buffered_body_.size();

  if (bytes_remaining_in_buffer_ > 0) {
    // Offload heavy distilling to another thread.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::TaskPriority::USER_BLOCKING},
        base::BindOnce(
            [](std::string data, std::unique_ptr<Rewriter> rewriter,
               const std::string& stylesheet) -> auto {
              SCOPED_UMA_HISTOGRAM_TIMER("Brave.Speedreader.Distill");
              int written = rewriter->Write(data.c_str(), data.length());
              // Error occurred
              if (written != 0) {
                return data;
              }

              rewriter->End();
              const std::string& transformed = rewriter->GetOutput();

              // TODO(brave-browser/issues/10372): would be better to pass
              // explicit signal back from rewriter to indicate if content was
              // found
              if (transformed.length() < 1024) {
                return data;
              }

              return stylesheet + transformed;
            },
            std::move(buffered_body_),
            rewriter_service_->MakeRewriter(response_url_),
            rewriter_service_->GetContentStylesheet()),
        base::BindOnce(&SpeedReaderURLLoader::CompleteLoading,
                       weak_factory_.GetWeakPtr()));
    return;
  }
  CompleteLoading(std::move(buffered_body_));
}

void SpeedReaderURLLoader::CompleteSending() {
  DCHECK_EQ(State::kSending, state_);
  state_ = State::kCompleted;
  // Call client's OnComplete() if |this|'s OnComplete() has already been
  // called.
  if (complete_status_.has_value()) {
    destination_url_loader_client_->OnComplete(complete_status_.value());
    // TODO(keur, iefremov): This API could probably be improved with an enum
    // indicating distill success, distill fail, load from cache.
    // |complete_status_| has an |exists_in_cache| field.
    if (delegate_)
      delegate_->OnDistillComplete();
  }

  body_consumer_watcher_.Cancel();
  body_producer_watcher_.Cancel();
  body_consumer_handle_.reset();
  body_producer_handle_.reset();
}

}  // namespace speedreader
