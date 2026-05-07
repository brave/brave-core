// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/oblivious_http/oblivious_http_chunk_handler.h"

#include "base/strings/string_number_conversions.h"

ObliviousHttpChunkHandler::ObliviousHttpChunkHandler(
    mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
        chunk_client_remote,
    base::OnceCallback<void(std::optional<std::string>)> on_request_complete)
    : bhttp_decoder_(this),
      on_request_complete_(std::move(on_request_complete)),
      chunk_client_(std::move(chunk_client_remote)) {}

ObliviousHttpChunkHandler::~ObliviousHttpChunkHandler() = default;

// static
std::unique_ptr<ObliviousHttpChunkHandler> ObliviousHttpChunkHandler::Create(
    const std::string& key_config_str,
    mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
        chunk_client_remote,
    base::OnceCallback<void(std::optional<std::string>)> on_request_complete) {
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

std::optional<std::string> ObliviousHttpChunkHandler::EncryptRequest(
    std::string_view plaintext) {
  DCHECK(ohttp_client_);
  auto result =
      ohttp_client_->EncryptRequestChunk(plaintext, /*is_final_chunk=*/true);
  if (!result.ok()) {
    return std::nullopt;
  }
  return std::move(*result);
}

void ObliviousHttpChunkHandler::OnDataReceived(std::string_view data,
                                               base::OnceClosure resume) {
  DCHECK(ohttp_client_);
  auto status = ohttp_client_->DecryptResponse(data, /*end_stream=*/false);
  if (!status.ok()) {
    has_error_ = true;
  }
  std::move(resume).Run();
}

void ObliviousHttpChunkHandler::OnComplete(bool success) {
  DCHECK(ohttp_client_);
  if (!success) {
    NotifyURLLoaderComplete(false);
    return;
  }
  auto status = ohttp_client_->DecryptResponse("", /*end_stream=*/true);
  if (!status.ok()) {
    NotifyURLLoaderComplete(false);
    return;
  }
  // On success, completion flows through OnChunksDone -> bhttp_decoder_.
  NotifyURLLoaderComplete(true);
}

void ObliviousHttpChunkHandler::OnRetry(base::OnceClosure /*start_retry*/) {}

absl::Status ObliviousHttpChunkHandler::OnDecryptedChunk(
    absl::string_view decrypted_chunk) {
  return bhttp_decoder_.Decode(decrypted_chunk, /*end_stream=*/false);
}

absl::Status ObliviousHttpChunkHandler::OnChunksDone() {
  auto status = bhttp_decoder_.Decode({}, /*end_stream=*/true);
  if (!status.ok()) {
    NotifyBHTTPComplete(false);
    return status;
  }
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnInformationalResponseStatusCode(
    uint16_t /*status_code*/) {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnInformationalResponseHeader(
    absl::string_view /*name*/,
    absl::string_view /*value*/) {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnInformationalResponseDone() {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnInformationalResponsesSectionDone() {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnFinalResponseStatusCode(
    uint16_t status_code) {
  inner_status_code_ = static_cast<int>(status_code);
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnFinalResponseHeader(
    absl::string_view name,
    absl::string_view value) {
  pending_headers_.emplace_back(std::string(name), std::string(value));
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnFinalResponseHeadersDone() {
  net::HttpResponseHeaders::Builder builder(
      net::HttpVersion(1, 1), base::NumberToString(inner_status_code_));
  for (const auto& [name, value] : pending_headers_) {
    builder.AddHeader(name, value);
  }
  headers_ = builder.Build();
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnBodyChunk(
    absl::string_view body_chunk) {
  chunk_client_->OnBodyChunk(std::string(body_chunk));
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnBodyChunksDone() {
  NotifyBHTTPComplete(true);
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnTrailer(absl::string_view /*name*/,
                                                  absl::string_view /*value*/) {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkHandler::OnTrailersDone() {
  return absl::OkStatus();
}

void ObliviousHttpChunkHandler::NotifyBHTTPComplete(bool success) {
  if (!success) {
    has_error_ = true;
  }
  bhttp_complete_ = true;
  if (url_loader_complete_) {
    RunCompleteCallback();
  }
}

void ObliviousHttpChunkHandler::NotifyURLLoaderComplete(bool success) {
  if (!success) {
    has_error_ = true;
    RunCompleteCallback();
    return;
  }
  url_loader_complete_ = true;
  if (bhttp_complete_) {
    RunCompleteCallback();
  }
}

void ObliviousHttpChunkHandler::RunCompleteCallback() {
  if (on_request_complete_) {
    std::move(on_request_complete_)
        .Run(has_error_ ? std::nullopt : std::optional<std::string>(""));
  }
}
