// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/third_party/quiche/src/quiche/binary_http/binary_http_message.h"
#include "net/third_party/quiche/src/quiche/oblivious_http/common/oblivious_http_chunk_handler.h"
#include "net/third_party/quiche/src/quiche/oblivious_http/oblivious_http_client.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "url/gurl.h"

namespace {

// Applies relay_request_headers to |headers| if present. If brave_services_key
// is set, also computes a Digest (SHA-256 of |encrypted_body|) and an
// Authorization header signed with that key, and adds both to |headers|.
void ApplyBraveRelayHeaders(const network::mojom::ObliviousHttpRequest& request,
                            const std::string& encrypted_body,
                            net::HttpRequestHeaders& headers) {
  if (request.relay_request_headers) {
    headers.MergeFrom(*request.relay_request_headers);
  }
  if (request.brave_services_key) {
    base::flat_map<std::string, std::string> signing_headers;
    const auto digest = brave_service_keys::GetDigestHeader(encrypted_body);
    signing_headers.emplace(digest.first, digest.second);
    const auto auth = brave_service_keys::GetAuthorizationHeader(
        *request.brave_services_key, signing_headers, request.relay_url,
        net::HttpRequestHeaders::kPostMethod, {"digest"});
    headers.SetHeader(digest.first, digest.second);
    headers.SetHeader(auth.first, auth.second);
  }
}

// Implements quiche::ObliviousHttpChunkHandler,
// quiche::BinaryHttpResponse::IndeterminateLengthDecoder::MessageSectionHandler,
// and network::SimpleURLLoaderStreamConsumer to support chunked OHTTP
// responses. Owned by RequestState and must not outlive it.
//
// Use Create() to construct — it handles key parsing and client creation.
// Call EncryptRequest() once after Create() to encrypt the request payload.
class ObliviousHttpChunkHandler
    : public quiche::ObliviousHttpChunkHandler,
      public quiche::BinaryHttpResponse::IndeterminateLengthDecoder::
          MessageSectionHandler,
      public network::SimpleURLLoaderStreamConsumer {
 public:
  ObliviousHttpChunkHandler(
      mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
          chunk_client_remote,
      base::OnceCallback<void(std::optional<std::string>)> on_request_complete)
      : chunk_client_(std::move(chunk_client_remote)),
        bhttp_decoder_(this),
        on_request_complete_(std::move(on_request_complete)) {}

  // Parses |key_config_str| and constructs the handler. Returns nullptr if
  // key parsing or client creation fails.
  //
  // |on_request_complete| maps directly to OnRequestComplete: called with
  // std::nullopt on stream failure (so the existing error-extraction path via
  // loader->NetError() runs) and with "" on success (so the chunked
  // completion path in BRAVE_OBLIVIOUS_HTTP_MAYBE_NOTIFY_COMPLETE_FROM_CHUNKS
  // runs).
  static std::unique_ptr<ObliviousHttpChunkHandler> Create(
      const std::string& key_config_str,
      mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
          chunk_client_remote,
      base::OnceCallback<void(std::optional<std::string>)>
          on_request_complete) {
    auto key_configs =
        quiche::ObliviousHttpKeyConfigs::ParseConcatenatedKeys(key_config_str);
    if (!key_configs.ok() || key_configs->NumKeys() == 0) {
      return nullptr;
    }
    auto key_config = key_configs->PreferredConfig();
    auto pub_key = key_configs->GetPublicKeyForId(key_config.GetKeyId());
    if (!pub_key.ok()) {
      return nullptr;
    }

    // Construct the handler first so we have a stable address to pass to
    // ChunkedObliviousHttpClient::Create, which holds a non-owning reference.
    auto handler = std::make_unique<ObliviousHttpChunkHandler>(
        std::move(chunk_client_remote), std::move(on_request_complete));

    auto client_result = quiche::ChunkedObliviousHttpClient::Create(
        *pub_key, key_config, handler.get());
    if (!client_result.ok()) {
      return nullptr;
    }

    handler->ohttp_client_.emplace(std::move(*client_result));
    return handler;
  }

  // Encrypts |plaintext| as a single final request chunk. Must be called after
  // Create() and before any response data arrives.
  std::optional<std::string> EncryptRequest(std::string_view plaintext) {
    DCHECK(ohttp_client_);
    auto result =
        ohttp_client_->EncryptRequestChunk(plaintext, /*is_final_chunk=*/true);
    if (!result.ok()) {
      return std::nullopt;
    }
    return std::move(*result);
  }

  ~ObliviousHttpChunkHandler() override = default;

  int inner_status_code() const { return inner_status_code_; }
  scoped_refptr<net::HttpResponseHeaders> headers() const { return headers_; }

  // network::SimpleURLLoaderStreamConsumer:
  void OnDataReceived(std::string_view data,
                      base::OnceClosure resume) override {
    DCHECK(ohttp_client_);
    auto status = ohttp_client_->DecryptResponse(data, /*end_stream=*/false);
    if (!status.ok()) {
      NotifyComplete(std::nullopt);
      return;
    }
    std::move(resume).Run();
  }

  void OnComplete(bool success) override {
    DCHECK(ohttp_client_);
    if (!success) {
      NotifyComplete(std::nullopt);
      return;
    }
    auto status = ohttp_client_->DecryptResponse({}, /*end_stream=*/true);
    if (!status.ok()) {
      NotifyComplete(std::nullopt);
    }
    // On success, completion flows through OnChunksDone -> bhttp_decoder_.
  }

  void OnRetry(base::OnceClosure /*start_retry*/) override {
    NotifyComplete(std::nullopt);
  }

  // quiche::ObliviousHttpChunkHandler:
  absl::Status OnDecryptedChunk(absl::string_view decrypted_chunk) override {
    return bhttp_decoder_.Decode(decrypted_chunk, /*end_stream=*/false);
  }

  absl::Status OnChunksDone() override {
    auto status = bhttp_decoder_.Decode({}, /*end_stream=*/true);
    if (!status.ok()) {
      NotifyComplete(std::nullopt);
      return status;
    }
    NotifyComplete("");
    return absl::OkStatus();
  }

  // quiche::BinaryHttpResponse::IndeterminateLengthDecoder::MessageSectionHandler:
  absl::Status OnInformationalResponseStatusCode(
      uint16_t /*status_code*/) override {
    return absl::OkStatus();
  }
  absl::Status OnInformationalResponseHeader(
      absl::string_view /*name*/,
      absl::string_view /*value*/) override {
    return absl::OkStatus();
  }
  absl::Status OnInformationalResponseDone() override {
    return absl::OkStatus();
  }
  absl::Status OnInformationalResponsesSectionDone() override {
    return absl::OkStatus();
  }
  absl::Status OnFinalResponseStatusCode(uint16_t status_code) override {
    inner_status_code_ = static_cast<int>(status_code);
    return absl::OkStatus();
  }
  absl::Status OnFinalResponseHeader(absl::string_view name,
                                     absl::string_view value) override {
    pending_headers_.emplace_back(std::string(name), std::string(value));
    return absl::OkStatus();
  }
  absl::Status OnFinalResponseHeadersDone() override {
    net::HttpResponseHeaders::Builder builder(
        net::HttpVersion(1, 1), base::NumberToString(inner_status_code_));
    for (const auto& [name, value] : pending_headers_) {
      builder.AddHeader(name, value);
    }
    headers_ = builder.Build();
    return absl::OkStatus();
  }
  absl::Status OnBodyChunk(absl::string_view body_chunk) override {
    chunk_client_->OnBodyChunk(std::string(body_chunk));
    return absl::OkStatus();
  }
  absl::Status OnBodyChunksDone() override { return absl::OkStatus(); }
  absl::Status OnTrailer(absl::string_view /*name*/,
                         absl::string_view /*value*/) override {
    return absl::OkStatus();
  }
  absl::Status OnTrailersDone() override { return absl::OkStatus(); }

 private:
  void NotifyComplete(std::optional<std::string> response) {
    if (on_request_complete_) {
      std::move(on_request_complete_).Run(std::move(response));
    }
  }

  mojo::Remote<network::mojom::ObliviousHttpChunkClient> chunk_client_;
  // Populated in Create() after the handler has a stable address.
  std::optional<quiche::ChunkedObliviousHttpClient> ohttp_client_;
  quiche::BinaryHttpResponse::IndeterminateLengthDecoder bhttp_decoder_;

  int inner_status_code_ = 0;
  scoped_refptr<net::HttpResponseHeaders> headers_;
  std::vector<std::pair<std::string, std::string>> pending_headers_;

  base::OnceCallback<void(std::optional<std::string>)> on_request_complete_;
};

}  // namespace

// BRAVE_OBLIVIOUS_HTTP_REQUEST_STATE_MEMBERS adds the chunked-OHTTP handler
// field to ObliviousHttpRequestHandler::RequestState.
#define BRAVE_OBLIVIOUS_HTTP_REQUEST_STATE_MEMBERS \
  std::unique_ptr<ObliviousHttpChunkHandler> chunk_handler;

// BRAVE_OBLIVIOUS_HTTP_MAYBE_ENCRYPT_AS_CHUNKED runs before non-chunked
// encryption in ContinueHandlingRequest. When chunking is enabled it delegates
// key parsing and client creation to ObliviousHttpChunkHandler::Create, then
// encrypts the request payload via EncryptRequest(). The encrypted blob is
// stored in a function-local for use in
// BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST.
#define BRAVE_OBLIVIOUS_HTTP_MAYBE_ENCRYPT_AS_CHUNKED                        \
  std::optional<std::string> chunked_encrypted_blob;                         \
  if (state->request->enable_chunking) {                                     \
    CHECK(state->request->chunk_client);                                     \
    state->chunk_handler = ObliviousHttpChunkHandler::Create(                \
        state->request->key_config, std::move(state->request->chunk_client), \
        base::BindOnce(&ObliviousHttpRequestHandler::OnRequestComplete,      \
                       base::Unretained(this), id));                         \
    if (!state->chunk_handler) {                                             \
      RespondWithError(id, net::ERR_FAILED,                                  \
                       /*outer_response_error_code=*/std::nullopt);          \
      return;                                                                \
    }                                                                        \
    chunked_encrypted_blob =                                                 \
        state->chunk_handler->EncryptRequest(padded_payload);                \
    if (!chunked_encrypted_blob) {                                           \
      RespondWithError(id, net::ERR_FAILED,                                  \
                       /*outer_response_error_code=*/std::nullopt);          \
      return;                                                                \
    }                                                                        \
  }

// BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST applies Brave relay headers.
// When chunking is enabled it overwrites maybe_encrypted_blob with the chunked
// ciphertext so that the upstream AttachStringForUpload call (and any
// Digest/Authorization signing) uses the correct bytes.
#define BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST           \
  if (state->request->enable_chunking) {                         \
    *maybe_encrypted_blob = std::move(*chunked_encrypted_blob);  \
  }                                                              \
  ApplyBraveRelayHeaders(*state->request, *maybe_encrypted_blob, \
                         resource_request->headers);

// BRAVE_OBLIVIOUS_HTTP_MAYBE_DOWNLOAD_AS_STREAM runs after the loader has
// been fully configured (upload body set, timeout set). When chunking is
// enabled it switches to streaming and returns early; DownloadToString is
// skipped.
#define BRAVE_OBLIVIOUS_HTTP_MAYBE_DOWNLOAD_AS_STREAM            \
  if (state->request->enable_chunking) {                         \
    state->loader->DownloadAsStream(GetURLLoaderFactory(),       \
                                    state->chunk_handler.get()); \
    return;                                                      \
  }

// BRAVE_OBLIVIOUS_HTTP_MAYBE_NOTIFY_COMPLETE_FROM_CHUNKS runs in
// OnRequestComplete before decryption. When chunking is enabled and the
// response string is empty (posted by OnChunksDone), we skip decryption and
// call NotifyComplete directly. A nullopt response falls through to the
// existing error-extraction path.
#define BRAVE_OBLIVIOUS_HTTP_MAYBE_NOTIFY_COMPLETE_FROM_CHUNKS    \
  if (state->request->enable_chunking) {                          \
    CHECK(response && response->empty());                         \
    NotifyComplete(id, state->chunk_handler->inner_status_code(), \
                   state->chunk_handler->headers(), /*body=*/{}); \
    return;                                                       \
  }

#include <services/network/oblivious_http_request_handler.cc>

#undef BRAVE_OBLIVIOUS_HTTP_MAYBE_NOTIFY_COMPLETE_FROM_CHUNKS
#undef BRAVE_OBLIVIOUS_HTTP_MAYBE_DOWNLOAD_AS_STREAM
#undef BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST
#undef BRAVE_OBLIVIOUS_HTTP_MAYBE_ENCRYPT_AS_CHUNKED
#undef BRAVE_OBLIVIOUS_HTTP_REQUEST_STATE_MEMBERS
