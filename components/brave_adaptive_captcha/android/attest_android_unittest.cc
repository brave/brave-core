/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/android/attest_android.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=AttestAndroidTest.*

namespace brave_adaptive_captcha {

namespace {

constexpr char kServerHost[] = "https://grants.rewards.brave.com";
constexpr char kStartUrl[] =
    "https://grants.rewards.brave.com/v1/attestations/android";
constexpr char kAttestUrl[] =
    "https://grants.rewards.brave.com/v1/attestations/android/payment_id";
constexpr char kSolveUrl[] =
    "https://grants.rewards.brave.com/v3/captcha/solution/payment_id/"
    "captcha_id";

struct CapturedRequest {
  std::string method;
  GURL url;
  std::string body;
};

}  // namespace

class AttestAndroidTest : public testing::Test {
 public:
  AttestAndroidTest() {
    ServerUtil::GetInstance()->SetServerHostForTesting(kServerHost);
    attest_ = std::make_unique<AttestAndroid>(
        test_url_loader_factory_.GetSafeWeakWrapper());
    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this](const network::ResourceRequest& request) {
          requests_.push_back(
              {request.method, request.url, network::GetUploadData(request)});
        }));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  std::unique_ptr<AttestAndroid> attest_;
  std::vector<CapturedRequest> requests_;
};

TEST_F(AttestAndroidTest, StartAttestationSuccess) {
  test_url_loader_factory_.AddResponse(kStartUrl, R"({"uniqueValue":"nonce"})",
                                       net::HTTP_CREATED);

  base::test::TestFuture<std::string> future;
  attest_->StartAttestation(
      "payment_id",
      base::BindLambdaForTesting([&](const std::string& unique_value) {
        future.SetValue(unique_value);
      }));

  EXPECT_EQ(future.Get(), "nonce");
  ASSERT_EQ(requests_.size(), 1u);
  EXPECT_EQ(requests_[0].method, "POST");
  EXPECT_EQ(requests_[0].url, kStartUrl);
  EXPECT_EQ(base::test::ParseJson(requests_[0].body),
            base::test::ParseJson(R"({"paymentId":"payment_id"})"));
}

TEST_F(AttestAndroidTest, StartAttestationHttpError) {
  test_url_loader_factory_.AddResponse(kStartUrl, "",
                                       net::HTTP_INTERNAL_SERVER_ERROR);

  base::test::TestFuture<std::string> future;
  attest_->StartAttestation(
      "payment_id",
      base::BindLambdaForTesting([&](const std::string& unique_value) {
        future.SetValue(unique_value);
      }));

  EXPECT_EQ(future.Get(), "");
}

TEST_F(AttestAndroidTest, StartAttestationMissingUniqueValue) {
  test_url_loader_factory_.AddResponse(kStartUrl, "{}", net::HTTP_CREATED);

  base::test::TestFuture<std::string> future;
  attest_->StartAttestation(
      "payment_id",
      base::BindLambdaForTesting([&](const std::string& unique_value) {
        future.SetValue(unique_value);
      }));

  EXPECT_EQ(future.Get(), "");
}

TEST_F(AttestAndroidTest, AttestPaymentIdSolvesCaptcha) {
  test_url_loader_factory_.AddResponse(kAttestUrl, "", net::HTTP_OK);
  test_url_loader_factory_.AddResponse(kSolveUrl, "", net::HTTP_OK);

  base::test::TestFuture<bool> future;
  attest_->AttestPaymentId("payment_id", "captcha_id", "integrity_token",
                           "nonce", "package_name", future.GetCallback());

  EXPECT_TRUE(future.Get());
  ASSERT_EQ(requests_.size(), 2u);

  EXPECT_EQ(requests_[0].method, "PUT");
  EXPECT_EQ(requests_[0].url, kAttestUrl);
  EXPECT_EQ(base::test::ParseJson(requests_[0].body),
            base::test::ParseJson(R"({"integrityToken":"integrity_token",
                                      "uniqueValue":"nonce",
                                      "packageName":"package_name"})"));

  EXPECT_EQ(requests_[1].method, "POST");
  EXPECT_EQ(requests_[1].url, kSolveUrl);
  EXPECT_EQ(base::test::ParseJson(requests_[1].body),
            base::test::ParseJson(R"({"solution":"payment_id"})"));
}

TEST_F(AttestAndroidTest, AttestPaymentIdRejectedDoesNotSolve) {
  test_url_loader_factory_.AddResponse(kAttestUrl, "", net::HTTP_UNAUTHORIZED);

  base::test::TestFuture<bool> future;
  attest_->AttestPaymentId("payment_id", "captcha_id", "integrity_token",
                           "nonce", "package_name", future.GetCallback());

  EXPECT_FALSE(future.Get());
  // The captcha solution request must not be issued when attestation fails.
  ASSERT_EQ(requests_.size(), 1u);
  EXPECT_EQ(requests_[0].url, kAttestUrl);
}

TEST_F(AttestAndroidTest, AttestPaymentIdSolveFails) {
  test_url_loader_factory_.AddResponse(kAttestUrl, "", net::HTTP_OK);
  test_url_loader_factory_.AddResponse(kSolveUrl, "",
                                       net::HTTP_INTERNAL_SERVER_ERROR);

  base::test::TestFuture<bool> future;
  attest_->AttestPaymentId("payment_id", "captcha_id", "integrity_token",
                           "nonce", "package_name", future.GetCallback());

  EXPECT_FALSE(future.Get());
  ASSERT_EQ(requests_.size(), 2u);
  EXPECT_EQ(requests_[1].url, kSolveUrl);
}

}  // namespace brave_adaptive_captcha
