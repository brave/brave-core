// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/e2ee_processor.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/e2ee/lib.rs.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::_;
using testing::NiceMock;

namespace ai_chat {

namespace {

constexpr char kModelName[] = "glm5";
constexpr char kExpectedPath[] = "/v1/models/glm5/attestation";
constexpr char kSuccessJson[] = R"({"model_public_key": "test-public-key"})";

}  // namespace

class E2EEProcessorTest : public testing::Test {
 public:
  void SetUp() override {
    processor_ = std::make_unique<E2EEProcessor>(nullptr);
    auto mock =
        std::make_unique<NiceMock<api_request_helper::MockAPIRequestHelper>>(
            TRAFFIC_ANNOTATION_FOR_TESTS, nullptr);
    mock_ = mock.get();
    processor_->SetAPIRequestHelperForTesting(std::move(mock));
  }

 protected:
  void SetUpMock(int times,
                 net::HttpStatusCode response_code,
                 base::Value body) {
    EXPECT_CALL(*mock_,
                Request(_, testing::Property(&GURL::path, kExpectedPath), _, _,
                        _, _, _, _))
        .Times(times)
        .WillRepeatedly(
            [response_code,
             body = std::make_shared<base::Value>(std::move(body))](
                const std::string&, const GURL&, const std::string&,
                const std::string&,
                api_request_helper::APIRequestHelper::ResultCallback callback,
                const base::flat_map<std::string, std::string>&,
                const api_request_helper::APIRequestOptions&,
                api_request_helper::APIRequestHelper::
                    ResponseConversionCallback) {
              std::move(callback).Run(api_request_helper::APIRequestResult(
                  response_code, body->Clone(), {}, net::OK, GURL()));
              return api_request_helper::APIRequestHelper::Ticket();
            });
  }

  std::optional<mojom::APIError> FetchAndGetError() {
    std::optional<mojom::APIError> result;
    base::RunLoop run_loop;
    processor_->FetchModelAttestation(
        kModelName,
        base::BindLambdaForTesting([&](std::optional<mojom::APIError> error) {
          result = error;
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  // Injects a hex-encoded public key into the attestation cache via a mock
  // fetch, and returns the key hex unchanged.
  std::string InjectAttestation(const std::string& model_public_key_hex) {
    const std::string json =
        R"({"model_public_key": ")" + model_public_key_hex + R"("})";
    SetUpMock(1, net::HTTP_OK, base::Value(base::test::ParseJsonDict(json)));
    auto error = FetchAndGetError();
    EXPECT_FALSE(error.has_value());
    return model_public_key_hex;
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  std::unique_ptr<E2EEProcessor> processor_;
  raw_ptr<api_request_helper::MockAPIRequestHelper> mock_;
};

TEST_F(E2EEProcessorTest, FetchModelAttestation_Success) {
  SetUpMock(1, net::HTTP_OK,
            base::Value(base::test::ParseJsonDict(kSuccessJson)));
  EXPECT_FALSE(FetchAndGetError().has_value());
}

TEST_F(E2EEProcessorTest, FetchModelAttestation_ServerError) {
  SetUpMock(1, net::HTTP_INTERNAL_SERVER_ERROR, base::Value());

  auto error = FetchAndGetError();
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(*error, mojom::APIError::ConnectionIssue);
}

TEST_F(E2EEProcessorTest, FetchModelAttestation_MissingKey) {
  SetUpMock(1, net::HTTP_OK, base::Value(base::test::ParseJsonDict(R"({})")));

  auto error = FetchAndGetError();
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(*error, mojom::APIError::InternalError);
}

// After a successful fetch, a second call should resolve immediately from cache
// without issuing another network request.
TEST_F(E2EEProcessorTest, FetchModelAttestation_UsesCache) {
  SetUpMock(1, net::HTTP_OK,
            base::Value(base::test::ParseJsonDict(kSuccessJson)));

  EXPECT_FALSE(FetchAndGetError().has_value());
  // Second call — resolves from cache, no further Request call.
  EXPECT_FALSE(FetchAndGetError().has_value());
}

// ClearModelAttestations forces a fresh network fetch on the next call.
TEST_F(E2EEProcessorTest, ClearModelAttestations_ForcesRefetch) {
  SetUpMock(2, net::HTTP_OK,
            base::Value(base::test::ParseJsonDict(kSuccessJson)));

  EXPECT_FALSE(FetchAndGetError().has_value());
  processor_->ClearModelAttestations();
  // After clearing, a new network request should be issued.
  EXPECT_FALSE(FetchAndGetError().has_value());
}

// After one hour the cache expires and a fresh fetch is issued.
TEST_F(E2EEProcessorTest, FetchModelAttestation_ExpiresAfterOneHour) {
  SetUpMock(2, net::HTTP_OK,
            base::Value(base::test::ParseJsonDict(kSuccessJson)));

  EXPECT_FALSE(FetchAndGetError().has_value());

  // Still within TTL — resolves from cache, no extra Request call needed.
  task_environment_.FastForwardBy(base::Minutes(55));
  EXPECT_FALSE(FetchAndGetError().has_value());

  // Past TTL — a fresh network request must be issued.
  task_environment_.FastForwardBy(base::Minutes(10));
  EXPECT_FALSE(FetchAndGetError().has_value());
}

// GenerateReceivingKeypair returns a 64-char hex public key (32 bytes) and
// an owned secret key box.
TEST_F(E2EEProcessorTest, GenerateReceivingKeypair_ReturnsValidKeypair) {
  auto keypair = processor_->GenerateReceivingKeypair();
  // 32 bytes → 64 hex chars.
  EXPECT_EQ(keypair.public_key_hex.size(), 64u);
  // Basic hex validity check.
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(base::HexStringToBytes(keypair.public_key_hex, &bytes));
  EXPECT_EQ(bytes.size(), 32u);
  // secret_key box is destroyed when keypair goes out of scope.
}

// Encrypt with a valid cached attestation returns a non-empty hex ciphertext.
TEST_F(E2EEProcessorTest, Encrypt_WithCachedAttestation_ReturnsHexCiphertext) {
  auto model_keypair = ai_chat::generate_receiving_keypair();
  const std::string model_pub_hex = base::HexEncode(std::vector<uint8_t>(
      model_keypair.public_key.begin(), model_keypair.public_key.end()));
  InjectAttestation(model_pub_hex);

  base::ListValue content;
  content.Append(base::Value("hello world"));

  auto encrypt_cb = processor_->CreateEncryptCallback(kModelName);
  const std::string ciphertext_hex = encrypt_cb.Run(content);
  EXPECT_FALSE(ciphertext_hex.empty());
  // Must be valid hex.
  std::vector<uint8_t> ciphertext_bytes;
  EXPECT_TRUE(base::HexStringToBytes(ciphertext_hex, &ciphertext_bytes));
}

// Encrypt without a cached attestation returns an empty string.
TEST_F(E2EEProcessorTest, Encrypt_WithMissingAttestation_ReturnsEmpty) {
  base::ListValue content;
  content.Append(base::Value("hello"));
  auto encrypt_cb = processor_->CreateEncryptCallback(kModelName);
  EXPECT_TRUE(encrypt_cb.Run(content).empty());
}

// Full decrypt round-trip: produce ciphertext encrypted with the client's
// public key (as the model would do), then decrypt it with the client secret.
TEST_F(E2EEProcessorTest, Decrypt_Roundtrip) {
  auto client_keypair = processor_->GenerateReceivingKeypair();

  std::vector<uint8_t> client_pub_bytes;
  ASSERT_TRUE(
      base::HexStringToBytes(client_keypair.public_key_hex, &client_pub_bytes));

  const std::string plaintext = "secret response from model";
  auto enc_result = ai_chat::encrypt(
      rust::Slice<const uint8_t>(
          reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size()),
      rust::Slice<const uint8_t>(client_pub_bytes.data(),
                                 client_pub_bytes.size()));
  ASSERT_TRUE(enc_result.error.empty()) << std::string(enc_result.error);

  const std::string ciphertext_hex = base::HexEncode(std::vector<uint8_t>(
      enc_result.ciphertext.begin(), enc_result.ciphertext.end()));

  auto decrypt_cb =
      processor_->CreateDecryptCallback(&*client_keypair.secret_key);
  const std::string result = decrypt_cb.Run(ciphertext_hex);
  EXPECT_EQ(result, plaintext);
  // client_keypair.secret_key box is destroyed when keypair goes out of scope.
}

// Decrypt with non-hex input returns an empty string.
TEST_F(E2EEProcessorTest, Decrypt_InvalidHex_ReturnsEmpty) {
  auto keypair = processor_->GenerateReceivingKeypair();
  auto decrypt_cb = processor_->CreateDecryptCallback(&*keypair.secret_key);
  EXPECT_TRUE(decrypt_cb.Run("not-valid-hex!").empty());
}

// Decrypt with a valid hex string that is too short returns an empty string.
TEST_F(E2EEProcessorTest, Decrypt_TooShortCiphertext_ReturnsEmpty) {
  auto keypair = processor_->GenerateReceivingKeypair();
  auto decrypt_cb = processor_->CreateDecryptCallback(&*keypair.secret_key);
  // 10 bytes — well below the 56-byte (32 + 24) minimum.
  EXPECT_TRUE(decrypt_cb.Run("00000000000000000000").empty());
}

// Two independent keypairs do not interfere with each other.
TEST_F(E2EEProcessorTest, TwoKeypairs_AreIndependent) {
  auto kp2 = processor_->GenerateReceivingKeypair();
  auto decrypt_cb2 = processor_->CreateDecryptCallback(&*kp2.secret_key);

  {
    // kp1 is destroyed at end of this block.
    auto kp1 = processor_->GenerateReceivingKeypair();
    (void)kp1;
  }

  // kp2's decrypt callback still works after kp1 is destroyed.
  EXPECT_TRUE(decrypt_cb2.Run("00000000000000000000").empty());
  // kp2.secret_key box is destroyed when kp2 goes out of scope.
}

}  // namespace ai_chat
