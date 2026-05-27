// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/oblivious_http/oblivious_http_chunk_processor.h"

#include "base/check.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"

namespace oblivious_http {

namespace {
constexpr size_t kMaxEncryptChunkSize = 16384;
}  // namespace

ObliviousHttpChunkProcessor::ObliviousHttpChunkProcessor(
    mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
        chunk_client_remote,
    base::OnceCallback<void(std::optional<std::string>)> on_request_complete)
    : bhttp_decoder_(this),
      on_request_complete_(std::move(on_request_complete)),
      chunk_client_(std::move(chunk_client_remote)) {}

ObliviousHttpChunkProcessor::~ObliviousHttpChunkProcessor() = default;

// static
std::unique_ptr<ObliviousHttpChunkProcessor>
ObliviousHttpChunkProcessor::Create(
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

  auto processor = base::WrapUnique(new ObliviousHttpChunkProcessor(
      std::move(chunk_client_remote), std::move(on_request_complete)));

  auto client_result = quiche::ChunkedObliviousHttpClient::Create(
      *pub_key, key_config, processor.get());
  if (!client_result.ok()) {
    return nullptr;
  }

  processor->ohttp_client_.emplace(std::move(*client_result));
  return processor;
}

std::optional<std::string> ObliviousHttpChunkProcessor::EncryptRequest(
    std::string_view plaintext) {
  CHECK(ohttp_client_);

  std::string result;
  for (size_t offset = 0; offset < plaintext.size();
       offset += kMaxEncryptChunkSize) {
    const bool is_final = offset + kMaxEncryptChunkSize >= plaintext.size();
    const auto chunk = plaintext.substr(offset, kMaxEncryptChunkSize);
    auto encrypted = ohttp_client_->EncryptRequestChunk(chunk, is_final);
    if (!encrypted.ok()) {
      return std::nullopt;
    }
    result += std::move(*encrypted);
  }
  return result;
}

void ObliviousHttpChunkProcessor::OnDataReceived(std::string_view data,
                                                 base::OnceClosure resume) {
  CHECK(ohttp_client_);
  auto status = ohttp_client_->DecryptResponse(data, /*end_stream=*/false);
  if (!status.ok()) {
    has_error_ = true;
  }
  std::move(resume).Run();
}

void ObliviousHttpChunkProcessor::OnComplete(bool success) {
  CHECK(ohttp_client_);
  if (!success) {
    NotifyURLLoaderComplete(/*success=*/false);
    return;
  }
  auto status = ohttp_client_->DecryptResponse("", /*end_stream=*/true);
  if (!status.ok()) {
    NotifyURLLoaderComplete(/*success=*/false);
    return;
  }
  // On success, completion flows through OnChunksDone -> bhttp_decoder_.
  NotifyURLLoaderComplete(/*success=*/true);
}

void ObliviousHttpChunkProcessor::OnRetry(base::OnceClosure /*start_retry*/) {}

absl::Status ObliviousHttpChunkProcessor::OnDecryptedChunk(
    absl::string_view decrypted_chunk) {
  return bhttp_decoder_.Decode(decrypted_chunk, /*end_stream=*/false);
}

absl::Status ObliviousHttpChunkProcessor::OnChunksDone() {
  auto status = bhttp_decoder_.Decode({}, /*end_stream=*/true);
  if (!status.ok()) {
    NotifyBHTTPComplete(/*success=*/false);
    return status;
  }
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnInformationalResponseStatusCode(
    uint16_t /*status_code*/) {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnInformationalResponseHeader(
    absl::string_view /*name*/,
    absl::string_view /*value*/) {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnInformationalResponseDone() {
  return absl::OkStatus();
}

absl::Status
ObliviousHttpChunkProcessor::OnInformationalResponsesSectionDone() {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnFinalResponseStatusCode(
    uint16_t status_code) {
  inner_status_code_ = static_cast<int>(status_code);
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnFinalResponseHeader(
    absl::string_view name,
    absl::string_view value) {
  pending_headers_.emplace_back(std::string(name), std::string(value));
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnFinalResponseHeadersDone() {
  net::HttpResponseHeaders::Builder builder(
      net::HttpVersion(1, 1), base::NumberToString(inner_status_code_));
  for (const auto& [name, value] : pending_headers_) {
    builder.AddHeader(name, value);
  }
  headers_ = builder.Build();
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnBodyChunk(
    absl::string_view body_chunk) {
  chunk_client_->OnBodyChunk(std::string(body_chunk));
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnBodyChunksDone() {
  NotifyBHTTPComplete(/*success=*/true);
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnTrailer(
    absl::string_view /*name*/,
    absl::string_view /*value*/) {
  return absl::OkStatus();
}

absl::Status ObliviousHttpChunkProcessor::OnTrailersDone() {
  return absl::OkStatus();
}

void ObliviousHttpChunkProcessor::NotifyBHTTPComplete(bool success) {
  if (!success) {
    has_error_ = true;
  }
  bhttp_complete_ = true;
  if (url_loader_complete_) {
    RunCompleteCallback();
  }
}

void ObliviousHttpChunkProcessor::NotifyURLLoaderComplete(bool success) {
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

void ObliviousHttpChunkProcessor::RunCompleteCallback() {
  if (on_request_complete_) {
    std::move(on_request_complete_)
        .Run(has_error_ ? std::nullopt : std::optional<std::string>(""));
  }
}

}  // namespace oblivious_http
