// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/oblivious_http/oblivious_http_chunk_processor.h"

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/http/http_status_code.h"
#include "net/third_party/quiche/src/quiche/binary_http/binary_http_message.h"
#include "net/third_party/quiche/src/quiche/oblivious_http/common/oblivious_http_header_key_config.h"
#include "net/third_party/quiche/src/quiche/oblivious_http/oblivious_http_gateway.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace oblivious_http {

namespace {

// Same key pair used in services/network/oblivious_http_request_handler tests.
// Key ID 1, X25519/HKDF-SHA256/AES-256-GCM.
constexpr uint8_t kTestPrivateKey[] = {
    0xff, 0x1f, 0x47, 0xb1, 0x68, 0xb6, 0xb9, 0xea, 0x65, 0xf7, 0x97,
    0x4f, 0xf2, 0x2e, 0xf2, 0x36, 0x94, 0xe2, 0xf6, 0xb6, 0x8d, 0x66,
    0xf3, 0xa7, 0x64, 0x14, 0x28, 0xd4, 0x45, 0x35, 0x01, 0x8f,
};

constexpr uint8_t kTestPublicKey[] = {
    0xa1, 0x5f, 0x40, 0x65, 0x86, 0xfa, 0xc4, 0x7b, 0x99, 0x59, 0x70,
    0xf1, 0x85, 0xd9, 0xd8, 0x91, 0xc7, 0x4d, 0xcf, 0x1e, 0xb9, 0x1a,
    0x7d, 0x50, 0xa5, 0x8b, 0x01, 0x68, 0x3e, 0x60, 0x05, 0x2d,
};

constexpr char kTestRequestBody[] = "request body";

constexpr char kLargeJsonResponse[] = R"({
  "status": "ok",
  "results": [
    {"id": 1, "name": "alpha",   "score": 98.6,  "active": true,  "tags": ["a", "b", "c"]},
    {"id": 2, "name": "beta",    "score": 72.1,  "active": false, "tags": ["d", "e"]},
    {"id": 3, "name": "gamma",   "score": 55.0,  "active": true,  "tags": ["f"]},
    {"id": 4, "name": "delta",   "score": 88.4,  "active": true,  "tags": ["g", "h", "i", "j"]},
    {"id": 5, "name": "epsilon", "score": 10.25, "active": false, "tags": []},
    {"id": 6, "name": "zeta",    "score": 63.9,  "active": true,  "tags": ["k", "l"]},
    {"id": 7, "name": "eta",     "score": 41.7,  "active": false, "tags": ["m", "n", "o"]},
    {"id": 8, "name": "theta",   "score": 99.99, "active": true,  "tags": ["p"]}
  ],
  "metadata": {
    "page": 1,
    "per_page": 8,
    "total": 8,
    "generated_at": "2026-05-08T00:00:00Z"
  }
})";

// Splits |s| into |n| roughly equal substrings.
std::vector<std::string> SplitIntoChunks(const std::string& s,
                                         size_t chunk_count) {
  std::vector<std::string> chunks;
  size_t chunk_size = std::max(size_t{1}, s.size() / chunk_count);
  for (size_t i = 0; i < s.size(); i += chunk_size) {
    chunks.push_back(s.substr(i, chunk_size));
  }
  return chunks;
}

}  // namespace

class ObliviousHttpChunkProcessorTest
    : public testing::Test,
      public quiche::ObliviousHttpChunkHandler,
      public network::mojom::ObliviousHttpChunkClient {
 public:
  void SetUp() override { ResetDecoder(); }

 protected:
  using CompleteFuture = base::test::TestFuture<std::optional<std::string>>;

  CompleteFuture ResetDecoder() {
    received_server_response_body_.clear();
    received_client_request_body_.clear();

    gateway_.emplace(
        quiche::ChunkedObliviousHttpGateway::Create(
            absl::string_view(reinterpret_cast<const char*>(kTestPrivateKey),
                              sizeof(kTestPrivateKey)),
            key_config_, this)
            .value());

    absl::string_view pub_key(reinterpret_cast<const char*>(kTestPublicKey),
                              sizeof(kTestPublicKey));
    std::string key_config_string =
        quiche::ObliviousHttpKeyConfigs::Create(key_config_, pub_key)
            .value()
            .GenerateConcatenatedKeys()
            .value();

    chunk_client_receiver_.reset();
    CompleteFuture future;
    processor_ = ObliviousHttpChunkProcessor::Create(
        key_config_string, chunk_client_receiver_.BindNewPipeAndPassRemote(),
        future.GetCallback());
    EXPECT_NE(nullptr, processor_);
    return future;
  }

  // quiche::ObliviousHttpChunkHandler:
  absl::Status OnDecryptedChunk(absl::string_view chunk) override {
    received_client_request_body_ += chunk;
    return absl::OkStatus();
  }
  absl::Status OnChunksDone() override { return absl::OkStatus(); }

  // network::mojom::ObliviousHttpChunkClient:
  void OnBodyChunk(const std::string& chunk) override {
    received_server_response_body_ += chunk;
  }

  std::string BuildGatewayResponse(
      const std::string& body,
      net::HttpStatusCode status,
      std::vector<std::pair<std::string, std::string>> headers,
      size_t bhttp_chunk_count = 1) {
    quiche::BinaryHttpResponse::IndeterminateLengthEncoder bhttp_encoder;
    std::vector<quiche::BinaryHttpMessage::FieldView> field_views;
    for (const auto& [name, value] : headers) {
      field_views.push_back({name, value});
    }

    auto encoded_headers = bhttp_encoder.EncodeHeaders(status, field_views);
    CHECK(encoded_headers.ok());

    auto encrypted_headers =
        gateway_->EncryptResponse(*encoded_headers, /*is_final_chunk=*/false);
    CHECK(encrypted_headers.ok());

    std::string result = *encrypted_headers;

    // Encode and encrypt each body slice, marking only the last as final.
    auto slices = SplitIntoChunks(body, bhttp_chunk_count);
    for (size_t i = 0; i < slices.size(); ++i) {
      bool is_last = (i == slices.size() - 1);
      auto encoded_body = bhttp_encoder.EncodeBodyChunks(
          {slices[i]}, /*body_chunks_done=*/is_last);
      CHECK(encoded_body.ok());

      auto encrypted_body =
          gateway_->EncryptResponse(*encoded_body, /*is_final_chunk=*/is_last);
      CHECK(encrypted_body.ok());

      result += *encrypted_body;
    }

    return result;
  }

  void EncryptRequestAndGatewayDecrypt() {
    auto encrypted_request = processor_->EncryptRequest(kTestRequestBody);
    ASSERT_TRUE(encrypted_request.has_value());
    ASSERT_TRUE(
        gateway_->DecryptRequest(*encrypted_request, /*end_stream=*/true).ok());
    EXPECT_EQ(kTestRequestBody, received_client_request_body_);
  }

  // Drives a full round-trip: encrypts kTestRequestBody, feeds the simulated
  // gateway response through the decoder's stream consumer interface, and
  // returns the future resolved by the processor's completion callback.
  CompleteFuture RunRoundTrip(
      const std::string& response_body,
      net::HttpStatusCode status,
      std::vector<std::pair<std::string, std::string>> headers,
      size_t bhttp_chunk_count = 1,
      size_t network_chunk_count = 1) {
    CompleteFuture future = ResetDecoder();

    EncryptRequestAndGatewayDecrypt();

    std::string response_bytes = BuildGatewayResponse(
        response_body, status, std::move(headers), bhttp_chunk_count);

    for (const auto& network_chunk :
         SplitIntoChunks(response_bytes, network_chunk_count)) {
      base::test::TestFuture<void> data_future;
      processor_->OnDataReceived(network_chunk, data_future.GetCallback());
      EXPECT_TRUE(data_future.Wait());
    }

    processor_->OnComplete(/*success=*/true);
    chunk_client_receiver_.FlushForTesting();
    return future;
  }

  base::test::TaskEnvironment task_environment_;

  quiche::ObliviousHttpHeaderKeyConfig key_config_{
      quiche::ObliviousHttpHeaderKeyConfig::Create(
          /*key_id=*/1,
          EVP_HPKE_DHKEM_X25519_HKDF_SHA256,
          EVP_HPKE_HKDF_SHA256,
          EVP_HPKE_AES_256_GCM)
          .value()};
  std::optional<quiche::ChunkedObliviousHttpGateway> gateway_;

  mojo::Receiver<network::mojom::ObliviousHttpChunkClient>
      chunk_client_receiver_{this};
  std::unique_ptr<ObliviousHttpChunkProcessor> processor_;
  std::string received_server_response_body_;
  std::string received_client_request_body_;
};

TEST_F(ObliviousHttpChunkProcessorTest, Create_BadKeyConfig_ReturnsNull) {
  mojo::Receiver<network::mojom::ObliviousHttpChunkClient> receiver{this};
  EXPECT_EQ(nullptr,
            ObliviousHttpChunkProcessor::Create(
                "", receiver.BindNewPipeAndPassRemote(), base::DoNothing()));

  receiver.reset();
  EXPECT_EQ(nullptr,
            ObliviousHttpChunkProcessor::Create(
                "not a valid key config", receiver.BindNewPipeAndPassRemote(),
                base::DoNothing()));
}

TEST_F(ObliviousHttpChunkProcessorTest,
       EncryptRequest_GatewayDecryptsCorrectly) {
  EncryptRequestAndGatewayDecrypt();
}

TEST_F(ObliviousHttpChunkProcessorTest,
       RoundTrip_BodyChunksDeliveredToChunkClient) {
  auto future = RunRoundTrip("hello chunked", net::HTTP_OK,
                             {{"content-type", "text/plain"}});

  EXPECT_TRUE(future.Get().has_value());
  EXPECT_EQ("hello chunked", received_server_response_body_);
}

TEST_F(ObliviousHttpChunkProcessorTest,
       RoundTrip_StatusCodeAndHeadersPopulated) {
  auto future = RunRoundTrip("body", net::HTTP_OK,
                             {{"content-type", "application/json"},
                              {"x-custom-header", "custom-value"}});

  EXPECT_TRUE(future.Get().has_value());
  EXPECT_EQ(200, processor_->inner_status_code());
  ASSERT_NE(nullptr, processor_->headers());
  EXPECT_EQ("application/json",
            processor_->headers()->GetNormalizedHeader("content-type"));
  EXPECT_EQ("custom-value",
            processor_->headers()->GetNormalizedHeader("x-custom-header"));
}

TEST_F(ObliviousHttpChunkProcessorTest,
       OnDataReceived_InvalidData_OnCompleteYieldsEmptyBody) {
  auto future = ResetDecoder();
  EncryptRequestAndGatewayDecrypt();

  base::test::TestFuture<void> data_future;
  processor_->OnDataReceived("not valid ohttp data", data_future.GetCallback());
  ASSERT_TRUE(data_future.Wait());

  processor_->OnComplete(/*success=*/true);
  chunk_client_receiver_.FlushForTesting();

  EXPECT_FALSE(future.Get().has_value());
  EXPECT_TRUE(received_server_response_body_.empty());
}

TEST_F(ObliviousHttpChunkProcessorTest,
       OnDataReceived_ValidOhttpInvalidBhttp_OnCompleteYieldsEmptyBody) {
  auto future = ResetDecoder();
  EncryptRequestAndGatewayDecrypt();

  // Encrypt garbage bytes as a valid ohttp chunk, bypassing bhttp encoding.
  auto encrypted = gateway_->EncryptResponse("not valid bhttp",
                                             /*is_final_chunk=*/true);
  ASSERT_TRUE(encrypted.ok());

  base::test::TestFuture<void> data_future;
  processor_->OnDataReceived(*encrypted, data_future.GetCallback());
  ASSERT_TRUE(data_future.Wait());

  processor_->OnComplete(/*success=*/true);
  chunk_client_receiver_.FlushForTesting();

  EXPECT_FALSE(future.Get().has_value());
  EXPECT_TRUE(received_server_response_body_.empty());
}

TEST_F(ObliviousHttpChunkProcessorTest,
       OnDataReceived_TruncatedPayload_OnCompleteFailureYieldsPartialBody) {
  auto future = ResetDecoder();
  EncryptRequestAndGatewayDecrypt();

  std::string response_bytes = BuildGatewayResponse(
      kLargeJsonResponse, net::HTTP_OK, {{"content-type", "application/json"}},
      /*bhttp_chunk_count=*/10);
  // Send only the first half of the encrypted payload.
  std::string truncated = response_bytes.substr(0, response_bytes.size() / 2);

  base::test::TestFuture<void> data_future;
  processor_->OnDataReceived(truncated, data_future.GetCallback());
  ASSERT_TRUE(data_future.Wait());

  processor_->OnComplete(/*success=*/false);
  chunk_client_receiver_.FlushForTesting();

  EXPECT_FALSE(future.Get().has_value());
  EXPECT_FALSE(received_server_response_body_.empty());
  EXPECT_LT(received_server_response_body_.size(),
            std::string_view(kLargeJsonResponse).size());
}

TEST_F(ObliviousHttpChunkProcessorTest,
       OnComplete_Failure_InvokesCallbackWithNullopt) {
  auto future = ResetDecoder();
  EncryptRequestAndGatewayDecrypt();

  processor_->OnComplete(/*success=*/false);

  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(ObliviousHttpChunkProcessorTest,
       RoundTrip_MixedChunkCounts_BodyReassembledCorrectly) {
  // Fewer bhttp chunks than network chunks: bhttp body spans multiple network
  // deliveries.
  auto future1 = RunRoundTrip(
      kLargeJsonResponse, net::HTTP_OK, {{"content-type", "application/json"}},
      /*bhttp_chunk_count=*/2, /*network_chunk_count=*/10);
  EXPECT_TRUE(future1.Get().has_value());
  EXPECT_EQ(kLargeJsonResponse, received_server_response_body_);

  // More bhttp chunks than network chunks: multiple bhttp chunks arrive in a
  // single network delivery.
  auto future2 = RunRoundTrip(
      kLargeJsonResponse, net::HTTP_OK, {{"content-type", "application/json"}},
      /*bhttp_chunk_count=*/10, /*network_chunk_count=*/2);
  EXPECT_TRUE(future2.Get().has_value());
  EXPECT_EQ(kLargeJsonResponse, received_server_response_body_);
}

}  // namespace oblivious_http
