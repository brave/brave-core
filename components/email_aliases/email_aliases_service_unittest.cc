// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/email_aliases/email_aliases_service.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/email_aliases/features.h"
#include "components/grit/brave_components_strings.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace email_aliases {

using AuthenticationStatus = email_aliases::mojom::AuthenticationStatus;

namespace {

// Test observer for authentication state changes
class TestObserver : public email_aliases::mojom::EmailAliasesServiceObserver {
 public:
  void OnAuthStateChanged(email_aliases::mojom::AuthStatePtr state) override {
    last_state = state->status;
  }
  bool WaitFor(AuthenticationStatus expected) {
    return base::test::RunUntil([&]() { return last_state == expected; });
  }
  void OnAliasesUpdated(std::vector<email_aliases::mojom::AliasPtr>) override {}
  AuthenticationStatus last_state = AuthenticationStatus::kUnauthenticated;
  mojo::Receiver<email_aliases::mojom::EmailAliasesServiceObserver> receiver_{
      this};
  void BindReceiver(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesServiceObserver>
          pending) {
    receiver_.Bind(std::move(pending));
  }
};

}  // namespace

class EmailAliasesServiceTest : public ::testing::Test {
 protected:
  EmailAliasesServiceTest() {
    feature_list_.InitAndEnableFeature(email_aliases::kEmailAliases);
  }

  void SetUp() override {
    url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);
    service_ = std::make_unique<EmailAliasesService>(url_loader_factory_);
    observer_ = std::make_unique<TestObserver>();
    mojo::PendingRemote<email_aliases::mojom::EmailAliasesServiceObserver>
        remote;
    observer_->BindReceiver(remote.InitWithNewPipeAndPassReceiver());
    service_->AddObserver(std::move(remote));
  }

  // Make authentication request and wait for the response.
  // Returns the error string if any, or std::nullopt on success.
  std::optional<std::string> RequestAuthenticationWithResponse(
      const std::string& email,
      const std::string& response_body) {
    test_url_loader_factory_.AddResponse(
        service_->GetAccountsServiceVerifyInitURL().spec(), response_body);
    bool called = false;
    std::optional<std::string> error;
    service_->RequestAuthentication(
        email, base::BindOnce(
                   [](bool* called, std::optional<std::string>* error,
                      const std::optional<std::string>& result) {
                     *called = true;
                     *error = result;
                   },
                   &called, &error));
    EXPECT_TRUE(base::test::RunUntil([&]() { return called; }));
    return error;
  }

  void CancelAuthenticationOrLogout() {
    const auto expected =
        email_aliases::mojom::AuthenticationStatus::kUnauthenticated;
    bool logout_called = false;
    service_->CancelAuthenticationOrLogout(
        base::BindOnce([](bool* called) { *called = true; }, &logout_called));
    EXPECT_TRUE(
        base::test::RunUntil([&logout_called]() { return logout_called; }));
    EXPECT_TRUE(observer_->WaitFor(expected));
  }

  void CallRequestAuthenticationAndCheck(
      const std::string& email,
      const std::string& response_body,
      AuthenticationStatus expected_status,
      const std::optional<std::string>& expected_error = std::nullopt) {
    const auto expected = expected_status;
    auto error = RequestAuthenticationWithResponse(email, response_body);
    if (expected_error) {
      EXPECT_TRUE(error.has_value());
      EXPECT_EQ(*error, *expected_error);
    } else {
      EXPECT_FALSE(error.has_value());
    }
    EXPECT_TRUE(observer_->WaitFor(expected));
  }

  void RunRequestSessionTest(const std::vector<std::string>& responses,
                             AuthenticationStatus expected_status) {
    auto error = RequestAuthenticationWithResponse(
        "test@example.com", "{\"verificationToken\":\"token123\"}");
    EXPECT_FALSE(error.has_value());
    // After verify/init success we should transition to Authenticating.
    EXPECT_TRUE(observer_->WaitFor(AuthenticationStatus::kAuthenticating));
    if (expected_status == AuthenticationStatus::kAuthenticating) {
      return;
    }
    for (const auto& body : responses) {
      test_url_loader_factory_.AddResponse(
          service_->GetAccountsServiceVerifyResultURL().spec(), body);
    }
    EXPECT_TRUE(observer_->WaitFor(expected_status));
  }

  // Helper: enqueue init success with |token|, start auth for |email|, wait
  // for callback and transition to kAuthenticating.
  void StartAuthWithInitToken(const std::string& email,
                              const std::string& token) {
    const std::string body =
        std::string("{\"verificationToken\":\"") + token + "\"}";
    auto error = RequestAuthenticationWithResponse(email, body);
    EXPECT_FALSE(error.has_value());
    EXPECT_TRUE(observer_->WaitFor(AuthenticationStatus::kAuthenticating));
  }

  // Helper: enqueue a successful verify/result response with |token|, then
  // wait until Authenticated and assert the stored auth token matches.
  void CompleteResultWithSuccess(const std::string& token) {
    const std::string body =
        std::string("{\"authToken\":\"") + token +
        "\", \"verified\":true, \"service\":\"email-aliases\"}";
    test_url_loader_factory_.AddResponse(
        EmailAliasesService::GetAccountsServiceVerifyResultURL().spec(), body);
    EXPECT_TRUE(observer_->WaitFor(AuthenticationStatus::kAuthenticated));
    EXPECT_EQ(service_->GetAuthTokenForTesting(), token);
  }

  base::test::ScopedFeatureList feature_list_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<EmailAliasesService> service_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestObserver> observer_;
};

TEST_F(EmailAliasesServiceTest, RequestAuthentication_EmptyEmail) {
  CallRequestAuthenticationAndCheck(
      "", "dummy body", AuthenticationStatus::kUnauthenticated,
      l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_NO_EMAIL_PROVIDED));
}

TEST_F(EmailAliasesServiceTest, RequestAuthentication_InvalidJson) {
  CallRequestAuthenticationAndCheck(
      "test@example.com", "not a json", AuthenticationStatus::kUnauthenticated,
      l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_INVALID_RESPONSE_BODY));
}

TEST_F(EmailAliasesServiceTest, RequestAuthentication_NoVerificationToken) {
  CallRequestAuthenticationAndCheck(
      "test@example.com", "{\"foo\":\"bar\"}",
      AuthenticationStatus::kUnauthenticated,
      l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_NO_VERIFICATION_TOKEN));
}

TEST_F(EmailAliasesServiceTest, RequestAuthentication_Success) {
  CallRequestAuthenticationAndCheck("test@example.com",
                                    "{\"verificationToken\":\"token123\"}",
                                    AuthenticationStatus::kAuthenticating);
}

TEST_F(EmailAliasesServiceTest,
       RequestAuthentication_ServerErrorObject_NoVerificationToken) {
  CallRequestAuthenticationAndCheck(
      "test@example.com",
      "{\"error\":\"server_error\",\"code\":400,\"status\":400}",
      AuthenticationStatus::kUnauthenticated,
      l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_NO_VERIFICATION_TOKEN));
}

TEST_F(EmailAliasesServiceTest, RequestAuthentication_VerificationTokenEmpty) {
  // Empty verificationToken should be treated as missing token.
  CallRequestAuthenticationAndCheck(
      "test@example.com", "{\"verificationToken\":\"\"}",
      AuthenticationStatus::kUnauthenticated,
      l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_NO_VERIFICATION_TOKEN));
}

TEST_F(EmailAliasesServiceTest, RequestSession_Success) {
  RunRequestSessionTest({"{\"authToken\":\"auth456\", \"verified\":true, "
                         "\"service\":\"email-aliases\"}"},
                        AuthenticationStatus::kAuthenticated);
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "auth456");
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest, RequestSession_InvalidJson) {
  RunRequestSessionTest({"not a json",
                         "{\"authToken\":\"auth456\", \"verified\":true, "
                         "\"service\":\"email-aliases\"}"},
                        AuthenticationStatus::kAuthenticated);
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest, RequestSession_RetryOnMissingAuthToken) {
  RunRequestSessionTest(
      {
          "{\"authToken\":null, \"verified\":false, "
          "\"service\":\"email-aliases\"}",  // triggers retry
          "{\"authToken\":\"auth456\", \"verified\":true, "
          "\"service\":\"email-aliases\"}"  // success
      },
      email_aliases::mojom::AuthenticationStatus::kAuthenticated);
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "auth456");
  // unauthenticated, authenticating, authenticated
  EXPECT_EQ(observer_->last_state,
            email_aliases::mojom::AuthenticationStatus::kAuthenticated);
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest, RequestSession_StopsOnVerificationFailed) {
  RunRequestSessionTest(
      {"{\"error\":\"verification_failed\", \"code\":400, \"status\":400}"},
      email_aliases::mojom::AuthenticationStatus::kUnauthenticated);
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "");
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest,
       RequestSession_ParseableWrongShape_RetriesAuthenticating) {
  // JSON parses but doesn't match expected schema; treated as retry.
  RunRequestSessionTest({"{}",
                         "{\"authToken\":\"auth456\", \"verified\":true, "
                         "\"service\":\"email-aliases\"}"},
                        AuthenticationStatus::kAuthenticated);
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest,
       RequestSession_VerifiedTrueMissingAuthToken_Retries) {
  // Verified true but no authToken provided; treated as retry.
  RunRequestSessionTest({"{\"verified\":true, \"service\":\"email-aliases\"}",
                         "{\"authToken\":\"auth456\", \"verified\":true, "
                         "\"service\":\"email-aliases\"}"},
                        AuthenticationStatus::kAuthenticated);
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest,
       RequestSession_NoResponseBody_RetriesAuthenticating) {
  // Start auth and simulate the first verify/result response having no body.
  const GURL result_url =
      EmailAliasesService::GetAccountsServiceVerifyResultURL();
  int result_request_count = 0;
  test_url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        if (request.url == result_url) {
          ++result_request_count;
        }
      }));

  // Queue init success and first result failure (no body via network error).
  test_url_loader_factory_.AddResponse(
      EmailAliasesService::GetAccountsServiceVerifyInitURL().spec(),
      "{\"verificationToken\":\"token123\"}");
  network::URLLoaderCompletionStatus status(net::ERR_FAILED);
  test_url_loader_factory_.AddResponse(result_url,
                                       network::mojom::URLResponseHead::New(),
                                       std::string(), status);

  // Start authentication and assert we are Authenticating.
  CallRequestAuthenticationAndCheck("test@example.com",
                                    "{\"verificationToken\":\"token123\"}",
                                    AuthenticationStatus::kAuthenticating);

  // Wait until the first result request has occurred and completed.
  EXPECT_TRUE(
      base::test::RunUntil([&]() { return result_request_count >= 1; }));

  // We should still be authenticating and have no auth token.
  EXPECT_EQ(observer_->last_state, AuthenticationStatus::kAuthenticating);
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "");
  EXPECT_TRUE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest,
       CancelAuthenticationOrLogout_ClearsStateAndNotifies) {
  // Authenticate
  RunRequestSessionTest(
      {"{\"authToken\":\"auth456\", \"verified\":true, "
       "\"service\":\"email-aliases\"}"},
      email_aliases::mojom::AuthenticationStatus::kAuthenticated);
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "auth456");

  // Now log out
  CancelAuthenticationOrLogout();

  // Auth token should be cleared
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "");
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest,
       CancelAuthenticationOrLogout_WhileAuthenticating) {
  RunRequestSessionTest(
      {"{\"authentication\":\"pending\"}"},
      email_aliases::mojom::AuthenticationStatus::kAuthenticating);

  CancelAuthenticationOrLogout();

  EXPECT_EQ(service_->GetAuthTokenForTesting(), "");
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest, Cancel_IgnoresLateCallback) {
  // Begin auth which immediately posts a verify/result request.
  StartAuthWithInitToken("cancel@example.com", "tokenCancel");

  // Cancel before any result response is delivered.
  CancelAuthenticationOrLogout();

  // Deliver a late success result; it should be ignored as the loader was
  // reset.
  test_url_loader_factory_.AddResponse(
      EmailAliasesService::GetAccountsServiceVerifyResultURL().spec(),
      "{\"authToken\":\"should_not_apply\",\"verified\":true,\"service\":"
      "\"email-aliases\"}");

  // State remains unauthenticated and no token is stored.
  EXPECT_EQ(observer_->last_state, AuthenticationStatus::kUnauthenticated);
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "");
  EXPECT_FALSE(service_->IsPollingSessionForTesting());
}

TEST_F(EmailAliasesServiceTest,
       AddObserver_InitialUnauthenticatedStateDelivered) {
  auto second_observer = std::make_unique<TestObserver>();
  mojo::PendingRemote<email_aliases::mojom::EmailAliasesServiceObserver> remote;
  second_observer->BindReceiver(remote.InitWithNewPipeAndPassReceiver());
  service_->AddObserver(std::move(remote));
  EXPECT_TRUE(second_observer->WaitFor(AuthenticationStatus::kUnauthenticated));
}

TEST_F(EmailAliasesServiceTest, Reauth_CancelsInFlightInit) {
  // First auth is in-flight (no init response yet); second auth proceeds.
  bool called1 = false;
  service_->RequestAuthentication(
      "first@example.com",
      base::BindOnce([](bool* called,
                        const std::optional<std::string>&) { *called = true; },
                     &called1));

  // Issue second auth first, then enqueue its init response so the response
  // is consumed by the second (active) request and not the first.
  bool called2 = false;
  service_->RequestAuthentication(
      "second@example.com",
      base::BindOnce([](bool* called,
                        const std::optional<std::string>&) { *called = true; },
                     &called2));
  test_url_loader_factory_.AddResponse(
      EmailAliasesService::GetAccountsServiceVerifyInitURL().spec(),
      "{\"verificationToken\":\"token2\"}");
  EXPECT_TRUE(base::test::RunUntil([&]() { return called2; }));
  EXPECT_TRUE(observer_->WaitFor(AuthenticationStatus::kAuthenticating));
  CompleteResultWithSuccess("auth456");
}

TEST_F(EmailAliasesServiceTest, ReauthDuringPolling_CancelsOldFlow) {
  // First flow reaches Authenticating and makes a pending result request.
  std::vector<std::string> auth_headers;
  const GURL result_url =
      EmailAliasesService::GetAccountsServiceVerifyResultURL();
  test_url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        if (request.url != result_url) {
          return;
        }
        if (auto header = request.headers.GetHeader("Authorization")) {
          auth_headers.push_back(*header);
        }
      }));
  StartAuthWithInitToken("a@example.com", "tokenA");
  test_url_loader_factory_.AddResponse(
      EmailAliasesService::GetAccountsServiceVerifyResultURL().spec(),
      "{\"authToken\":null, \"verified\":false, "
      "\"service\":\"email-aliases\"}");

  // Now re-auth; old timer and loader should be cancelled.
  StartAuthWithInitToken("b@example.com", "tokenB");

  // Complete with success; subsequent poll must use tokenB.
  CompleteResultWithSuccess("auth789");
  ASSERT_FALSE(auth_headers.empty());
  EXPECT_EQ(auth_headers.back(), "Bearer tokenB");
}

TEST_F(EmailAliasesServiceTest, Reauth_DoubleClick_CancelsEarlierFlow) {
  bool called1 = false;
  service_->RequestAuthentication(
      "x@example.com",
      base::BindOnce([](bool* called,
                        const std::optional<std::string>&) { *called = true; },
                     &called1));

  StartAuthWithInitToken("x@example.com", "tokenX");

  CompleteResultWithSuccess("authX");
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "authX");
  EXPECT_FALSE(called1);
}

TEST_F(EmailAliasesServiceTest, Reauth_NewTokenUsedForSubsequentPolls) {
  // Start first flow and return pending result.
  StartAuthWithInitToken("z1@example.com", "t1");
  test_url_loader_factory_.AddResponse(
      EmailAliasesService::GetAccountsServiceVerifyResultURL().spec(),
      "{\"authToken\":null, \"verified\":false, "
      "\"service\":\"email-aliases\"}");

  // Reauth with a new token and complete successfully.
  StartAuthWithInitToken("z2@example.com", "t2");
  CompleteResultWithSuccess("auth2");
}

// Timing-specific tests use a separate fixture with MOCK_TIME to validate
// rate limiting and total polling duration.
class EmailAliasesServiceTimingTest : public ::testing::Test {
 protected:
  EmailAliasesServiceTimingTest()
      : url_loader_factory_(/*no args*/),
        url_loader_wrapper_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    feature_list_.InitAndEnableFeature(email_aliases::kEmailAliases);
    service_ = std::make_unique<EmailAliasesService>(url_loader_wrapper_);
    observer_ = std::make_unique<TestObserver>();
    mojo::PendingRemote<email_aliases::mojom::EmailAliasesServiceObserver>
        remote;
    observer_->BindReceiver(remote.InitWithNewPipeAndPassReceiver());
    service_->AddObserver(std::move(remote));
  }

  // Starts auth and captures verify/result request times via interceptor.
  void StartAuthAndCaptureRequests(const std::string& init_body,
                                   const std::string& result_body) {
    // Intercept verify/result requests and record timestamps.
    const GURL verify_result_url =
        EmailAliasesService::GetAccountsServiceVerifyResultURL();
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          if (request.url == verify_result_url) {
            verify_result_request_times_.push_back(
                task_environment_.NowTicks());
          }
        }));

    // Provide canned responses for init and result endpoints.
    url_loader_factory_.AddResponse(
        EmailAliasesService::GetAccountsServiceVerifyInitURL().spec(),
        init_body);
    url_loader_factory_.AddResponse(verify_result_url.spec(), result_body);

    // Kick off authentication.
    bool called = false;
    std::optional<std::string> error;
    service_->RequestAuthentication(
        "test@example.com",
        base::BindOnce(
            [](bool* called_out, std::optional<std::string>* out,
               const std::optional<std::string>& result) {
              *called_out = true;
              *out = result;
            },
            &called, &error));
    EXPECT_TRUE(base::test::RunUntil([&]() { return called; }));
    EXPECT_FALSE(error.has_value());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::test::ScopedFeatureList feature_list_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_wrapper_;
  std::unique_ptr<EmailAliasesService> service_;
  std::unique_ptr<TestObserver> observer_;
  std::vector<base::TimeTicks> verify_result_request_times_;
};

TEST_F(EmailAliasesServiceTimingTest, VerifyResult_IsRateLimitedByTwoSeconds) {
  // First poll happens immediately; subsequent polls are delayed by 2s.
  const std::string init_body = "{\"verificationToken\":\"token123\"}";
  const std::string pending_body =
      "{\"authToken\":null,\"service\":\"email-aliases\",\"verified\":false}";
  StartAuthAndCaptureRequests(init_body, pending_body);

  // Wait for the first verify/result request.
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return verify_result_request_times_.size() >= 1; }));

  // Advance exactly 2 seconds to allow the next scheduled poll.
  task_environment_.FastForwardBy(base::Seconds(2));

  // Wait for the second request to be issued.
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return verify_result_request_times_.size() >= 2; }));

  ASSERT_GE(verify_result_request_times_.size(), 2u);
  const base::TimeDelta delta =
      verify_result_request_times_[1] - verify_result_request_times_[0];
  EXPECT_GE(delta, base::Seconds(2));
}

TEST_F(EmailAliasesServiceTimingTest, VerifyResult_StopsAfterMaxDuration) {
  // Provide a perpetually pending response; after 30 minutes total, polling
  // should stop and we should transition to Unauthenticated with an error.
  const std::string init_body = "{\"verificationToken\":\"token123\"}";
  const std::string pending_body =
      "{\"authToken\":null,\"service\":\"email-aliases\",\"verified\":false}";
  StartAuthAndCaptureRequests(init_body, pending_body);

  // 30 minutes * 60 seconds / 2 seconds + 2 = 902
  const unsigned long expected_requests = 30 * 60 / 2 + 2;

  EXPECT_EQ(verify_result_request_times_.size(), expected_requests);

  EXPECT_EQ(observer_->last_state,
            email_aliases::mojom::AuthenticationStatus::kUnauthenticated);
}

}  // namespace email_aliases
