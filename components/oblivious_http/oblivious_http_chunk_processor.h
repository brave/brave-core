// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OBLIVIOUS_HTTP_OBLIVIOUS_HTTP_CHUNK_PROCESSOR_H_
#define BRAVE_COMPONENTS_OBLIVIOUS_HTTP_OBLIVIOUS_HTTP_CHUNK_PROCESSOR_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_response_headers.h"
#include "net/third_party/quiche/src/quiche/binary_http/binary_http_message.h"
#include "net/third_party/quiche/src/quiche/oblivious_http/common/oblivious_http_chunk_handler.h"
#include "net/third_party/quiche/src/quiche/oblivious_http/oblivious_http_client.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"

namespace oblivious_http {

// Integrates with the upstream ObliviousHttpRequestHandler to add support for
// chunked OHTTP. Owned by ObliviousHttpRequestHandler::RequestState and must
// not outlive it.
//
// Use Create() to construct — it handles key parsing and client creation.
// Call EncryptRequest() once after Create() to encrypt the request payload.
class ObliviousHttpChunkProcessor
    : public quiche::ObliviousHttpChunkHandler,
      public quiche::BinaryHttpResponse::IndeterminateLengthDecoder::
          MessageSectionHandler,
      public network::SimpleURLLoaderStreamConsumer {
 public:
  // Parses |key_config_str| and constructs the decoder. Returns nullptr if
  // key parsing or client creation fails.
  //
  // |on_request_complete| maps directly to OnRequestComplete: called with
  // std::nullopt on stream failure (so the existing error-extraction path via
  // loader->NetError() runs) and with "" on success (so the chunked
  // completion path in BRAVE_OBLIVIOUS_HTTP_MAYBE_NOTIFY_COMPLETE_FROM_CHUNKS
  // runs).
  static std::unique_ptr<ObliviousHttpChunkProcessor> Create(
      const std::string& key_config_str,
      mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
          chunk_client_remote,
      base::OnceCallback<void(std::optional<std::string>)> on_request_complete);

  // Encrypts |plaintext| as a single final request chunk. Must be called after
  // Create() and before any response data arrives.
  std::optional<std::string> EncryptRequest(std::string_view plaintext);

  ~ObliviousHttpChunkProcessor() override;

  int inner_status_code() const { return inner_status_code_; }
  scoped_refptr<net::HttpResponseHeaders> headers() const { return headers_; }

  // network::SimpleURLLoaderStreamConsumer:
  void OnDataReceived(std::string_view data, base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

  // quiche::ObliviousHttpChunkHandler:
  absl::Status OnDecryptedChunk(absl::string_view decrypted_chunk) override;
  absl::Status OnChunksDone() override;

  // quiche::BinaryHttpResponse::IndeterminateLengthDecoder::MessageSectionHandler:
  absl::Status OnInformationalResponseStatusCode(uint16_t status_code) override;
  absl::Status OnInformationalResponseHeader(absl::string_view name,
                                             absl::string_view value) override;
  absl::Status OnInformationalResponseDone() override;
  absl::Status OnInformationalResponsesSectionDone() override;
  absl::Status OnFinalResponseStatusCode(uint16_t status_code) override;
  absl::Status OnFinalResponseHeader(absl::string_view name,
                                     absl::string_view value) override;
  absl::Status OnFinalResponseHeadersDone() override;
  absl::Status OnBodyChunk(absl::string_view body_chunk) override;
  absl::Status OnBodyChunksDone() override;
  absl::Status OnTrailer(absl::string_view name,
                         absl::string_view value) override;
  absl::Status OnTrailersDone() override;

 private:
  ObliviousHttpChunkProcessor(
      mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
          chunk_client_remote,
      base::OnceCallback<void(std::optional<std::string>)> on_request_complete);

  void NotifyBHTTPComplete(bool success);
  void NotifyURLLoaderComplete(bool success);
  void RunCompleteCallback();

  bool has_error_ = false;
  bool url_loader_complete_ = false;
  bool bhttp_complete_ = false;

  int inner_status_code_ = 0;
  scoped_refptr<net::HttpResponseHeaders> headers_;
  std::vector<std::pair<std::string, std::string>> pending_headers_;

  quiche::BinaryHttpResponse::IndeterminateLengthDecoder bhttp_decoder_;
  std::optional<quiche::ChunkedObliviousHttpClient> ohttp_client_;

  base::OnceCallback<void(std::optional<std::string>)> on_request_complete_;

  mojo::Remote<network::mojom::ObliviousHttpChunkClient> chunk_client_;
};

}  // namespace oblivious_http

#endif  // BRAVE_COMPONENTS_OBLIVIOUS_HTTP_OBLIVIOUS_HTTP_CHUNK_PROCESSOR_H_
