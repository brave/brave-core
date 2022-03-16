/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_url_loader.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/check.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/body_sniffer/body_sniffer_throttle.h"
#include "brave/components/speedreader/rust/ffi/speedreader.h"
#include "brave/components/speedreader/speedreader_result_delegate.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_throttle.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace speedreader {

namespace {

constexpr uint32_t kReadBufferSize = 32768;

}  // namespace

// static
std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
           mojo::PendingReceiver<network::mojom::URLLoaderClient>,
           SpeedReaderURLLoader*>
SpeedReaderURLLoader::CreateLoader(
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
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
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
    base::WeakPtr<SpeedreaderResultDelegate> delegate,
    const GURL& response_url,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    SpeedreaderRewriterService* rewriter_service)
    : body_sniffer::BodySnifferURLLoader(
          throttle,
          response_url,
          std::move(destination_url_loader_client),
          task_runner),
      delegate_(delegate),
      rewriter_service_(rewriter_service) {}

SpeedReaderURLLoader::~SpeedReaderURLLoader() = default;

void SpeedReaderURLLoader::OnBodyReadable(MojoResult) {
  DCHECK_EQ(State::kLoading, state_);

  if (!BodySnifferURLLoader::CheckBufferedBody(kReadBufferSize)) {
    return;
  }

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

void SpeedReaderURLLoader::OnCompleteSending() {
  // TODO(keur, iefremov): This API could probably be improved with an enum
  // indicating distill success, distill fail, load from cache.
  // |complete_status_| has an |exists_in_cache| field.
  if (delegate_)
    delegate_->OnDistillComplete();
}

}  // namespace speedreader
