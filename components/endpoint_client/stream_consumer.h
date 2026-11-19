/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_STREAM_CONSUMER_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_STREAM_CONSUMER_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"

class GURL;

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace endpoint_client::internal {

// The endpoint-independent callbacks of a streaming request. `on_complete`
// receives the body of a non-2xx response, if it was buffered.
struct RawStreamCallbacks {
  RawStreamCallbacks();
  RawStreamCallbacks(RawStreamCallbacks&&);
  RawStreamCallbacks& operator=(RawStreamCallbacks&&);
  ~RawStreamCallbacks();

  base::OnceCallback<void(scoped_refptr<net::HttpResponseHeaders> headers)>
      on_response_started;
  base::RepeatingCallback<void(std::string_view chunk)> on_chunk;
  base::OnceCallback<void(int net_error,
                          scoped_refptr<net::HttpResponseHeaders> headers,
                          std::optional<std::string> error_body)>
      on_complete;
};

// Streams the body of a 2xx response to `callbacks`, and buffers any other
// body, up to `max_error_body_size`, if `buffer_error_body`. Owns its
// `SimpleURLLoader`, so destroying it cancels the request. Each callback is
// the last thing its caller does, so any of them may destroy the consumer.
class StreamConsumer : public network::SimpleURLLoaderStreamConsumer {
 public:
  StreamConsumer(std::unique_ptr<network::SimpleURLLoader> simple_url_loader,
                 bool buffer_error_body,
                 std::optional<std::size_t> max_error_body_size,
                 RawStreamCallbacks callbacks);

  StreamConsumer(const StreamConsumer&) = delete;
  StreamConsumer& operator=(const StreamConsumer&) = delete;

  ~StreamConsumer() override;

  void Start(network::mojom::URLLoaderFactory& url_loader_factory);

 private:
  void OnResponseStarted(const GURL& final_url,
                         const network::mojom::URLResponseHead& response_head);

  // network::SimpleURLLoaderStreamConsumer:
  void OnDataReceived(std::string_view string_piece,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;
  const bool buffer_error_body_;
  const std::optional<std::size_t> max_error_body_size_;
  RawStreamCallbacks callbacks_;
  bool is_successful_ = false;
  std::string error_body_;
};

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_STREAM_CONSUMER_H_
