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

#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
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
constexpr char kSuccessJson[] =
    R"({"model_public_key": "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"})";

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

TEST_F(E2EEProcessorTest, FetchModelAttestation_UsesCache) {
  SetUpMock(1, net::HTTP_OK,
            base::Value(base::test::ParseJsonDict(kSuccessJson)));

  EXPECT_FALSE(FetchAndGetError().has_value());
  // Second call — resolves from cache, no further Request call.
  EXPECT_FALSE(FetchAndGetError().has_value());
}

TEST_F(E2EEProcessorTest, ClearModelAttestations_ForcesRefetch) {
  SetUpMock(2, net::HTTP_OK,
            base::Value(base::test::ParseJsonDict(kSuccessJson)));

  EXPECT_FALSE(FetchAndGetError().has_value());
  processor_->ClearCachedModelAttestations();
  // After clearing, a new network request should be issued.
  EXPECT_FALSE(FetchAndGetError().has_value());
}

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

TEST_F(E2EEProcessorTest, GenerateClientKeyPair_ReturnsValidKeypair) {
  auto keypair = processor_->GenerateClientKeyPair();
  // 32 bytes → 64 hex chars.
  EXPECT_EQ(keypair.public_key_hex.size(), 64u);
  // Basic hex validity check.
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(base::HexStringToBytes(keypair.public_key_hex, &bytes));
  EXPECT_EQ(bytes.size(), 32u);
}

TEST_F(E2EEProcessorTest, EncryptAndDecryptCallbacks) {
  // Generate a keypair to use as the model's public key.
  auto model_keypair = processor_->GenerateClientKeyPair();

  // Inject the attestation into the cache using the model's public key.
  base::DictValue attestation_body;
  attestation_body.Set("model_public_key", model_keypair.public_key_hex);
  SetUpMock(1, net::HTTP_OK, base::Value(std::move(attestation_body)));
  auto error = FetchAndGetError();
  EXPECT_FALSE(error.has_value());

  // Encrypt content using the model's public key via the callback.
  base::ListValue content;
  content.Append(base::Value("hello world"));
  auto encrypt_cb = processor_->CreateEncryptCallback(kModelName);
  auto encrypted = encrypt_cb.Run(content);
  ASSERT_TRUE(encrypted);
  EXPECT_FALSE(encrypted->empty());

  // Decrypt the encrypted content using the model's secret key.
  auto decrypt_cb =
      processor_->CreateDecryptCallback(&*model_keypair.secret_key);
  auto decrypted = decrypt_cb.Run(*encrypted);
  ASSERT_TRUE(decrypted);

  // The decrypted content should be the JSON-serialized ListValue.
  std::string expected_json;
  base::JSONWriter::Write(content, &expected_json);
  EXPECT_EQ(*decrypted, expected_json);
}

TEST_F(E2EEProcessorTest, Encrypt_WithMissingAttestation_Crashes) {
  base::ListValue content;
  content.Append(base::Value("hello"));
  EXPECT_DEATH(processor_->CreateEncryptCallback(kModelName), "");
}

TEST_F(E2EEProcessorTest, Decrypt_InvalidHex_ReturnsPlaintext) {
  auto keypair = processor_->GenerateClientKeyPair();
  auto decrypt_cb = processor_->CreateDecryptCallback(&*keypair.secret_key);
  auto result = decrypt_cb.Run("not-valid-hex!");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "not-valid-hex!");
}

TEST_F(E2EEProcessorTest, Decrypt_InvalidCiphertext_ReturnsEmpty) {
  auto keypair = processor_->GenerateClientKeyPair();
  auto decrypt_cb = processor_->CreateDecryptCallback(&*keypair.secret_key);

  // Ciphertext below the minimum (10 bytes).
  EXPECT_FALSE(decrypt_cb.Run(std::string(20, '0')).has_value());

  // Ciphertext at the minimum (56 bytes), but not actually valid encrypted
  // data.
  EXPECT_FALSE(decrypt_cb.Run(std::string(112, '0')).has_value());
}

}  // namespace ai_chat
