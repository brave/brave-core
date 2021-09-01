/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/get_adaptive_captcha_challenge.h"

#include <memory>
#include <string>

#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetAdaptiveCaptchaChallengeTest.*

namespace brave_adaptive_captcha {

class GetAdaptiveCaptchaChallengeTest : public testing::Test {
 public:
  GetAdaptiveCaptchaChallengeTest() {
    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            TRAFFIC_ANNOTATION_FOR_TESTS,
            test_url_loader_factory_.GetSafeWeakWrapper());
    challenge_ = std::make_unique<GetAdaptiveCaptchaChallenge>(
        api_request_helper_.get());
  }

  void OnGetChallengeServerOK(const std::string& captcha_id) {
    EXPECT_EQ(captcha_id, "ae07288c-d078-11eb-b8bc-0242ac130003");
    SignalUrlLoadCompleted();
  }

  void OnGetChallengeServerError404(const std::string& captcha_id) {
    EXPECT_EQ(captcha_id, "");
    SignalUrlLoadCompleted();
  }

  void OnGetChallengeServerError500(const std::string& captcha_id) {
    EXPECT_EQ(captcha_id, "");
    SignalUrlLoadCompleted();
  }

  void OnGetChallengeServerErrorRandom(const std::string& captcha_id) {
    EXPECT_EQ(captcha_id, "");
    SignalUrlLoadCompleted();
  }

 protected:
  network::TestURLLoaderFactory test_url_loader_factory_;
  std::unique_ptr<GetAdaptiveCaptchaChallenge> challenge_;

  void WaitForUrlLoadToComplete() {
    if (url_loaded_) {
      return;
    }

    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

 private:
  base::test::TaskEnvironment scoped_task_environment_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  std::unique_ptr<base::RunLoop> run_loop_;
  bool url_loaded_ = false;

  void SignalUrlLoadCompleted() {
    url_loaded_ = true;
    if (run_loop_) {
      run_loop_->Quit();
    }
  }
};

TEST_F(GetAdaptiveCaptchaChallengeTest, ServerOK) {
  test_url_loader_factory_.AddResponse(
      REWARDS_GRANT_STAGING_ENDPOINT "/v3/captcha/challenge/payment_id",
      "{ \"captchaID\": \"ae07288c-d078-11eb-b8bc-0242ac130003\" }",
      net::HTTP_OK);
  challenge_->Request(
      brave_adaptive_captcha::STAGING, "payment_id",
      base::BindOnce(&GetAdaptiveCaptchaChallengeTest::OnGetChallengeServerOK,
                     base::Unretained(this)));
  WaitForUrlLoadToComplete();
}

TEST_F(GetAdaptiveCaptchaChallengeTest, ServerError404) {
  test_url_loader_factory_.AddResponse(REWARDS_GRANT_STAGING_ENDPOINT
                                       "/v3/captcha/challenge/payment_id",
                                       "", net::HTTP_NOT_FOUND);
  challenge_->Request(
      brave_adaptive_captcha::STAGING, "payment_id",
      base::BindOnce(
          &GetAdaptiveCaptchaChallengeTest::OnGetChallengeServerError404,
          base::Unretained(this)));
  WaitForUrlLoadToComplete();
}

TEST_F(GetAdaptiveCaptchaChallengeTest, ServerError500) {
  test_url_loader_factory_.AddResponse(REWARDS_GRANT_STAGING_ENDPOINT
                                       "/v3/captcha/challenge/payment_id",
                                       "", net::HTTP_INTERNAL_SERVER_ERROR);
  challenge_->Request(
      brave_adaptive_captcha::STAGING, "payment_id",
      base::BindOnce(
          &GetAdaptiveCaptchaChallengeTest::OnGetChallengeServerError500,
          base::Unretained(this)));
  WaitForUrlLoadToComplete();
}

TEST_F(GetAdaptiveCaptchaChallengeTest, ServerErrorRandom) {
  test_url_loader_factory_.AddResponse(REWARDS_GRANT_STAGING_ENDPOINT
                                       "/v3/captcha/challenge/payment_id",
                                       "", net::HTTP_TOO_MANY_REQUESTS);
  challenge_->Request(
      brave_adaptive_captcha::STAGING, "payment_id",
      base::BindOnce(
          &GetAdaptiveCaptchaChallengeTest::OnGetChallengeServerErrorRandom,
          base::Unretained(this)));
  WaitForUrlLoadToComplete();
}

}  // namespace brave_adaptive_captcha
