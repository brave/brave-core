/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/origin_activation_limit_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_origin {

namespace {

constexpr char kOrderId[] = "3d5f19e2-4b71-49f5-966d-02111a067af1";
constexpr char kReceiptPayload[] =
    R"({"type":"android","raw_receipt":"token","package":"com.brave.browser",)"
    R"("subscription_id":"brave-origin"})";

constexpr char kCheckPathSuffix[] =
    "/credentials/batches/extend-with-receipt/check";
constexpr char kExtendPathSuffix[] = "/credentials/batches/extend-with-receipt";

}  // namespace

class OriginActivationLimitImplTest : public testing::Test {
 public:
  void SetUp() override {
    client_ = std::make_unique<OriginActivationLimitImpl>(
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_));
  }

  // Replies to the single pending request and returns it, so tests can assert
  // on the URL, method and body the client actually sent.
  network::ResourceRequest RespondToPendingRequest(
      net::HttpStatusCode status,
      const std::string& response_body) {
    EXPECT_EQ(1, url_loader_factory_.NumPending());
    const network::ResourceRequest request =
        url_loader_factory_.GetPendingRequest(0)->request;
    url_loader_factory_.SimulateResponseForPendingRequest(
        request.url.spec(), response_body, status);
    return request;
  }

  int num_pending() { return url_loader_factory_.NumPending(); }

 protected:
  base::test::TaskEnvironment task_environment_;
  // APIRequestHelper decodes responses out of process by default.
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<OriginActivationLimitImpl> client_;
};

TEST_F(OriginActivationLimitImplTest, CanExtendSendsReceiptToCheckEndpoint) {
  base::test::TestFuture<mojom::ExtendEligibilityPtr> future;
  client_->CanExtend(kOrderId, kReceiptPayload, future.GetCallback());

  const network::ResourceRequest request = RespondToPendingRequest(
      net::HTTP_OK, R"({"at_limit":true,"can_extend":true})");

  EXPECT_EQ("/v1/orders/" + std::string(kOrderId) + kCheckPathSuffix,
            request.url.path());
  // The whole receipt object is the body, unencoded - the service rejects a
  // base64 blob here, unlike /v1/orders/receipt.
  EXPECT_EQ(kReceiptPayload, request.request_body->elements()
                                 ->at(0)
                                 .As<network::DataElementBytes>()
                                 .AsStringPiece());

  const mojom::ExtendEligibilityPtr eligibility = future.Take();
  ASSERT_TRUE(eligibility);
  EXPECT_TRUE(eligibility->at_limit);
  EXPECT_TRUE(eligibility->can_extend);
}

TEST_F(OriginActivationLimitImplTest, CanExtendReportsNotAtLimit) {
  base::test::TestFuture<mojom::ExtendEligibilityPtr> future;
  client_->CanExtend(kOrderId, kReceiptPayload, future.GetCallback());
  RespondToPendingRequest(net::HTTP_OK,
                          R"({"at_limit":false,"can_extend":false})");

  const mojom::ExtendEligibilityPtr eligibility = future.Take();
  ASSERT_TRUE(eligibility);
  EXPECT_FALSE(eligibility->at_limit);
  EXPECT_FALSE(eligibility->can_extend);
}

// A response missing either field is not an answer to act on: defaulting it
// would either hide a real limit or offer an extension the service never
// promised.
TEST_F(OriginActivationLimitImplTest, CanExtendRejectsPartialResponse) {
  base::test::TestFuture<mojom::ExtendEligibilityPtr> future;
  client_->CanExtend(kOrderId, kReceiptPayload, future.GetCallback());
  RespondToPendingRequest(net::HTTP_OK, R"({"at_limit":true})");

  EXPECT_FALSE(future.Take());
}

TEST_F(OriginActivationLimitImplTest, CanExtendRejectsNonDictionaryResponse) {
  base::test::TestFuture<mojom::ExtendEligibilityPtr> future;
  client_->CanExtend(kOrderId, kReceiptPayload, future.GetCallback());
  RespondToPendingRequest(net::HTTP_OK, R"(["at_limit"])");

  EXPECT_FALSE(future.Take());
}

TEST_F(OriginActivationLimitImplTest, CanExtendRejectsErrorResponse) {
  base::test::TestFuture<mojom::ExtendEligibilityPtr> future;
  client_->CanExtend(kOrderId, kReceiptPayload, future.GetCallback());
  RespondToPendingRequest(net::HTTP_BAD_REQUEST,
                          R"({"at_limit":true,"can_extend":true})");

  EXPECT_FALSE(future.Take());
}

TEST_F(OriginActivationLimitImplTest, CanExtendWithoutReceiptSendsNoRequest) {
  base::test::TestFuture<mojom::ExtendEligibilityPtr> future;
  client_->CanExtend(kOrderId, /*receipt_payload=*/"", future.GetCallback());

  EXPECT_EQ(0, num_pending());
  EXPECT_FALSE(future.Take());
}

TEST_F(OriginActivationLimitImplTest, ExtendSucceedsOnSuccessResponse) {
  base::test::TestFuture<bool, const std::string&> future;
  client_->Extend(kOrderId, kReceiptPayload, future.GetCallback());

  const network::ResourceRequest request =
      RespondToPendingRequest(net::HTTP_OK, "{}");

  EXPECT_EQ("/v1/orders/" + std::string(kOrderId) + kExtendPathSuffix,
            request.url.path());
  EXPECT_TRUE(future.Get<0>());
  EXPECT_EQ("", future.Get<1>());
}

// Failures share the shape of every other payment service error, and the
// application error code is what tells a refusal worth showing the subscriber
// from one that only belongs in the log.
TEST_F(OriginActivationLimitImplTest, ExtendSurfacesApplicationErrorCode) {
  base::test::TestFuture<bool, const std::string&> future;
  client_->Extend(kOrderId, kReceiptPayload, future.GetCallback());
  RespondToPendingRequest(
      net::HTTP_UNPROCESSABLE_CONTENT,
      R"({"message":"not at limit","errorCode":"not_at_limit",)"
      R"("code":422,"data":{}})");

  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ("not_at_limit", future.Get<1>());
}

TEST_F(OriginActivationLimitImplTest, ExtendReportsFailureWithoutErrorCode) {
  base::test::TestFuture<bool, const std::string&> future;
  client_->Extend(kOrderId, kReceiptPayload, future.GetCallback());
  RespondToPendingRequest(net::HTTP_INTERNAL_SERVER_ERROR, "");

  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ("", future.Get<1>());
}

TEST_F(OriginActivationLimitImplTest, ExtendWithoutReceiptSendsNoRequest) {
  base::test::TestFuture<bool, const std::string&> future;
  client_->Extend(kOrderId, /*receipt_payload=*/"", future.GetCallback());

  EXPECT_EQ(0, num_pending());
  EXPECT_FALSE(future.Get<0>());
  EXPECT_EQ("", future.Get<1>());
}

}  // namespace brave_origin
