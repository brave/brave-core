// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string_view>

#include "brave/components/oblivious_http/oblivious_http_chunk_processor.h"
#include "brave/components/oblivious_http/utils.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

namespace {

constexpr std::string_view kChunkedObliviousHttpRequestMimeType =
    "message/ohttp-chunked-req";

}  // namespace

// BRAVE_OBLIVIOUS_HTTP_REQUEST_STATE_MEMBERS adds the chunked-OHTTP handler
// field to ObliviousHttpRequestHandler::RequestState.
#define BRAVE_OBLIVIOUS_HTTP_REQUEST_STATE_MEMBERS \
  std::unique_ptr<oblivious_http::ObliviousHttpChunkProcessor> chunk_handler;

// BRAVE_OBLIVIOUS_HTTP_MAYBE_ENCRYPT_AS_CHUNKED runs before non-chunked
// encryption in ContinueHandlingRequest. When chunking is enabled it delegates
// key parsing and client creation to ObliviousHttpChunkHandler::Create, then
// encrypts the request payload via EncryptRequest(). The encrypted blob is
// stored in a function-local for use in
// BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST.
#define BRAVE_OBLIVIOUS_HTTP_MAYBE_ENCRYPT_AS_CHUNKED                       \
  std::optional<std::string> chunked_encrypted_blob;                        \
  if (state->request->enable_chunking) {                                    \
    CHECK(state->request->chunk_client);                                    \
    state->chunk_handler =                                                  \
        oblivious_http::ObliviousHttpChunkProcessor::Create(                \
            state->request->key_config,                                     \
            std::move(state->request->chunk_client),                        \
            base::BindOnce(&ObliviousHttpRequestHandler::OnRequestComplete, \
                           base::Unretained(this), id));                    \
    if (!state->chunk_handler) {                                            \
      RespondWithError(id, net::ERR_FAILED,                                 \
                       /*outer_response_error_code=*/std::nullopt);         \
      return;                                                               \
    }                                                                       \
    chunked_encrypted_blob =                                                \
        state->chunk_handler->EncryptRequest(padded_payload);               \
    if (!chunked_encrypted_blob) {                                          \
      RespondWithError(id, net::ERR_FAILED,                                 \
                       /*outer_response_error_code=*/std::nullopt);         \
      return;                                                               \
    }                                                                       \
  }

// BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST applies Brave relay headers
// then, when chunking is enabled, handles the full streaming path: creates the
// loader, attaches the encrypted upload with the chunked MIME type, sets the
// timeout, calls DownloadAsStream, and returns early so the upstream
// DownloadToString path is never reached.
#define BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST                         \
  oblivious_http::ApplyBraveRelayHeaders(*state->request,                      \
                                         state->request->enable_chunking       \
                                             ? *chunked_encrypted_blob         \
                                             : *maybe_encrypted_blob,          \
                                         resource_request->headers);           \
  if (state->request->enable_chunking) {                                       \
    state->loader = SimpleURLLoader::Create(                                   \
        std::move(resource_request),                                           \
        net::NetworkTrafficAnnotationTag(state->request->traffic_annotation)); \
    state->loader->AttachStringForUpload(                                      \
        *chunked_encrypted_blob, kChunkedObliviousHttpRequestMimeType);        \
    state->loader->SetTimeoutDuration(state->request->timeout_duration         \
                                          ? *state->request->timeout_duration  \
                                          : kDefaultRequestTimeout);           \
    state->loader->DownloadAsStream(GetURLLoaderFactory(),                     \
                                    state->chunk_handler.get());               \
    return;                                                                    \
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
#undef BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST
#undef BRAVE_OBLIVIOUS_HTTP_MAYBE_ENCRYPT_AS_CHUNKED
#undef BRAVE_OBLIVIOUS_HTTP_REQUEST_STATE_MEMBERS
