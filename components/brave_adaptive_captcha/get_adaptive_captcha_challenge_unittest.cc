/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/get_adaptive_captcha_challenge.h"

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetAdaptiveCaptchaChallengeTest.*

namespace brave_adaptive_captcha {

class GetAdaptiveCaptchaChallengeTest : public testing::Test {
 public:
  GetAdaptiveCaptchaChallengeTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()),
        get_challenge_(std::make_unique<GetAdaptiveCaptchaChallenge>(
            &api_request_helper_)) {
    brave_adaptive_captcha::ServerUtil::GetInstance()->SetServerHostForTesting(
        "https://grants.rewards.brave.com");
  }

 protected:
  network::TestURLLoaderFactory test_url_loader_factory_;
  base::test::TaskEnvironment scoped_task_environment_;
  api_request_helper::APIRequestHelper api_request_helper_;
  std::unique_ptr<GetAdaptiveCaptchaChallenge> get_challenge_;
};

TEST_F(GetAdaptiveCaptchaChallengeTest, ServerOK) {
  test_url_loader_factory_.AddResponse(
      "https://grants.rewards.brave.com/v3/captcha/challenge/payment_id",
      "{ \"captchaID\": \"ae07288c-d078-11eb-b8bc-0242ac130003\" }",
      net::HTTP_OK);
  base::test::TestFuture<std::string> future;
  get_challenge_->Request("payment_id",
                          future.GetCallback<const std::string&>());
  EXPECT_EQ(future.Get(), "ae07288c-d078-11eb-b8bc-0242ac130003");
}

TEST_F(GetAdaptiveCaptchaChallengeTest, ServerError404) {
  test_url_loader_factory_.AddResponse(
      "https://grants.rewards.brave.com/v3/captcha/challenge/payment_id", "",
      net::HTTP_NOT_FOUND);
  base::test::TestFuture<std::string> future;
  get_challenge_->Request("payment_id",
                          future.GetCallback<const std::string&>());
  EXPECT_EQ(future.Get(), "");
}

TEST_F(GetAdaptiveCaptchaChallengeTest, ServerError500) {
  test_url_loader_factory_.AddResponse(
      "https://grants.rewards.brave.com/v3/captcha/challenge/payment_id", "",
      net::HTTP_INTERNAL_SERVER_ERROR);
  base::test::TestFuture<std::string> future;
  get_challenge_->Request("payment_id",
                          future.GetCallback<const std::string&>());
  EXPECT_EQ(future.Get(), "");
}

TEST_F(GetAdaptiveCaptchaChallengeTest, ServerErrorRandom) {
  test_url_loader_factory_.AddResponse(
      "https://grants.rewards.brave.com/v3/captcha/challenge/payment_id", "",
      net::HTTP_TOO_MANY_REQUESTS);
  base::test::TestFuture<std::string> future;
  get_challenge_->Request("payment_id",
                          future.GetCallback<const std::string&>());
  EXPECT_EQ(future.Get(), "");
}

}  // namespace brave_adaptive_captcha
