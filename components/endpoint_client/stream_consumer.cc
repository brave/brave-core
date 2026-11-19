/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/stream_consumer.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/notreached.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace endpoint_client::internal {

RawStreamCallbacks::RawStreamCallbacks() = default;

RawStreamCallbacks::RawStreamCallbacks(RawStreamCallbacks&&) = default;

RawStreamCallbacks& RawStreamCallbacks::operator=(RawStreamCallbacks&&) =
    default;

RawStreamCallbacks::~RawStreamCallbacks() = default;

StreamConsumer::StreamConsumer(
    std::unique_ptr<network::SimpleURLLoader> simple_url_loader,
    bool buffer_error_body,
    std::optional<std::size_t> max_error_body_size,
    RawStreamCallbacks callbacks)
    : simple_url_loader_(std::move(simple_url_loader)),
      buffer_error_body_(buffer_error_body),
      max_error_body_size_(max_error_body_size),
      callbacks_(std::move(callbacks)) {}

StreamConsumer::~StreamConsumer() = default;

void StreamConsumer::Start(
    network::mojom::URLLoaderFactory& url_loader_factory) {
  // `simple_url_loader_` is owned by `this`, so it can't run the callback after
  // `this` is destroyed.
  simple_url_loader_->SetOnResponseStartedCallback(base::BindOnce(
      &StreamConsumer::OnResponseStarted, base::Unretained(this)));
  simple_url_loader_->DownloadAsStream(&url_loader_factory, this);
}

void StreamConsumer::OnResponseStarted(
    const GURL& final_url,
    const network::mojom::URLResponseHead& response_head) {
  is_successful_ =
      response_head.headers &&
      network::IsSuccessfulStatus(response_head.headers->response_code());
  if (callbacks_.on_response_started) {
    std::move(callbacks_.on_response_started).Run(response_head.headers);
  }
}

void StreamConsumer::OnDataReceived(std::string_view string_piece,
                                    base::OnceClosure resume) {
  if (is_successful_) {
    std::move(resume).Run();
    callbacks_.on_chunk.Run(string_piece);
    return;
  }

  if (!buffer_error_body_) {
    std::move(resume).Run();
    return;
  }

  if (max_error_body_size_ &&
      string_piece.size() > *max_error_body_size_ - error_body_.size()) {
    simple_url_loader_.reset();
    std::move(callbacks_.on_complete)
        .Run(net::ERR_INSUFFICIENT_RESOURCES, nullptr, std::nullopt);
    return;
  }

  error_body_.append(string_piece);
  std::move(resume).Run();
}

void StreamConsumer::OnComplete(bool success) {
  const auto* response_info = simple_url_loader_->ResponseInfo();
  std::optional<std::string> error_body;
  if (!is_successful_ && buffer_error_body_) {
    error_body = std::move(error_body_);
  }
  std::move(callbacks_.on_complete)
      .Run(simple_url_loader_->NetError(),
           response_info ? response_info->headers : nullptr,
           std::move(error_body));
}

void StreamConsumer::OnRetry(base::OnceClosure start_retry) {
  // Retries aren't set up for streaming requests.
  NOTREACHED();
}

}  // namespace endpoint_client::internal
