// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/e2ee_processor.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
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

// ClearCachedModelAttestations forces a fresh network fetch on the next call.
TEST_F(E2EEProcessorTest, ClearCachedModelAttestations_ForcesRefetch) {
  SetUpMock(2, net::HTTP_OK,
            base::Value(base::test::ParseJsonDict(kSuccessJson)));

  EXPECT_FALSE(FetchAndGetError().has_value());
  processor_->ClearCachedModelAttestations();
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

}  // namespace ai_chat
