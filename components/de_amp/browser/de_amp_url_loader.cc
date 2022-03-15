/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_url_loader.h"

#include <utility>

#include "base/logging.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "brave/components/de_amp/browser/de_amp_throttle.h"
#include "brave/components/de_amp/browser/de_amp_util.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

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
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  mojo::PendingRemote<network::mojom::URLLoader> url_loader;
  mojo::PendingRemote<network::mojom::URLLoaderClient> url_loader_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient>
      url_loader_client_receiver =
          url_loader_client.InitWithNewPipeAndPassReceiver();

  auto loader = base::WrapUnique(
      new DeAmpURLLoader(std::move(throttle), response_url,
                         std::move(url_loader_client), std::move(task_runner)));
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
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : body_sniffer::BodySnifferURLLoader(
          throttle,
          response_url,
          std::move(destination_url_loader_client),
          task_runner),
      de_amp_throttle_(throttle) {}

DeAmpURLLoader::~DeAmpURLLoader() = default;

void DeAmpURLLoader::OnBodyReadable(MojoResult) {
  if (state_ == State::kSending) {
    // The pipe becoming readable when kSending means all buffered body has
    // already been sent.
    ForwardBodyToClient();
    return;
  }
  if (!CheckBufferedBody(kReadBufferSize)) {
    return;
  }

  if (!MaybeRedirectToCanonicalLink()) {
    CompleteLoading(std::move(buffered_body_));
  }

  body_consumer_watcher_.ArmOrNotify();
}

bool DeAmpURLLoader::MaybeRedirectToCanonicalLink() {
  std::string canonical_link;

  if (de_amp_throttle_ &&
      MaybeFindCanonicalAmpUrl(buffered_body_, &canonical_link)) {
    const GURL canonical_url(canonical_link);
    if (!VerifyCanonicalAmpUrl(canonical_url, response_url_)) {
      VLOG(2) << __func__ << " canonical link check failed " << canonical_url;
      return false;
    }
    VLOG(2) << __func__ << " de-amping and loading " << canonical_url;
    Abort();
    de_amp_throttle_->Redirect(canonical_url, response_url_);
    return true;
  } else {
    // Did not find AMP page and/or canonical link, load original
    return false;
  }
}

void DeAmpURLLoader::OnBodyWritable(MojoResult r) {
  DCHECK_EQ(State::kSending, state_);
  if (bytes_remaining_in_buffer_ > 0) {
    SendReceivedBodyToClient();
  } else {
    ForwardBodyToClient();
  }
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

}  // namespace de_amp
