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
#include "base/types/expected.h"
#include "brave/components/email_aliases/features.h"
#include "components/grit/brave_components_strings.h"
#include "net/base/net_errors.h"
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
                      base::expected<std::monostate, std::string> result) {
                     *called = true;
                     if (result.has_value()) {
                       *error = std::nullopt;
                     } else {
                       *error = result.error();
                     }
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

TEST_F(EmailAliasesServiceTest, RequestSession_Success) {
  RunRequestSessionTest({"{\"authToken\":\"auth456\", \"verified\":true, "
                         "\"service\":\"email-aliases\"}"},
                        AuthenticationStatus::kAuthenticated);
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "auth456");
}

TEST_F(EmailAliasesServiceTest, RequestSession_InvalidJson) {
  RunRequestSessionTest({"not a json"}, AuthenticationStatus::kAuthenticating);
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
}

TEST_F(EmailAliasesServiceTest, RequestSession_StopsOnVerificationFailed) {
  RunRequestSessionTest(
      {"{\"error\":\"verification_failed\", \"code\":400, \"status\":400}"},
      email_aliases::mojom::AuthenticationStatus::kUnauthenticated);
  EXPECT_EQ(service_->GetAuthTokenForTesting(), "");
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
}

TEST_F(EmailAliasesServiceTest,
       CancelAuthenticationOrLogout_WhileAuthenticating) {
  RunRequestSessionTest(
      {"{\"authentication\":\"pending\"}"},
      email_aliases::mojom::AuthenticationStatus::kAuthenticating);

  CancelAuthenticationOrLogout();

  EXPECT_EQ(service_->GetAuthTokenForTesting(), "");
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
               base::expected<std::monostate, std::string> result) {
              *called_out = true;
              if (result.has_value()) {
                *out = std::nullopt;
              } else {
                *out = result.error();
              }
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

// TODO(https://github.com/brave/brave-browser/issues/48696): Add tests for
// checking cancellation of polling, etc.

}  // namespace email_aliases
namespace email_aliases {

class EmailAliasesAPITest : public ::testing::Test {
 protected:
  EmailAliasesAPITest() {
    feature_list_.InitAndEnableFeature(email_aliases::kEmailAliases);
  }

  void SetUp() override {
    url_loader_wrapper_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    service_ = std::make_unique<EmailAliasesService>(url_loader_wrapper_);

    mojo::PendingRemote<email_aliases::mojom::EmailAliasesServiceObserver>
        remote;
    observer_.BindReceiver(remote.InitWithNewPipeAndPassReceiver());
    service_->AddObserver(std::move(remote));
  }

  // Minimal observer capturing alias updates.
  class AliasObserver
      : public email_aliases::mojom::EmailAliasesServiceObserver {
   public:
    void OnAuthStateChanged(email_aliases::mojom::AuthStatePtr) override {}
    void OnAliasesUpdated(
        std::vector<email_aliases::mojom::AliasPtr> aliases) override {
      ++alias_updates;
      last_aliases = std::move(aliases);
    }
    bool WaitForAliasUpdateCount(size_t count) {
      return base::test::RunUntil([&]() { return alias_updates >= count; });
    }

    size_t alias_updates = 0;
    std::vector<email_aliases::mojom::AliasPtr> last_aliases;
    mojo::Receiver<email_aliases::mojom::EmailAliasesServiceObserver> receiver{
        this};
    void BindReceiver(
        mojo::PendingReceiver<email_aliases::mojom::EmailAliasesServiceObserver>
            pending) {
      receiver.Bind(std::move(pending));
    }
  };

  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_wrapper_;
  std::unique_ptr<EmailAliasesService> service_;
  AliasObserver observer_;

  base::expected<std::string, std::string> CallGenerateAliasWith(
      const std::optional<std::string>& body,
      const std::optional<int>& net_error = std::nullopt) {
    const GURL manage_url =
        EmailAliasesService::GetEmailAliasesServiceBaseURL();
    if (net_error.has_value()) {
      network::URLLoaderCompletionStatus completion(*net_error);
      url_loader_factory_.AddResponse(manage_url, /*head=*/nullptr,
                                      /*content=*/"", completion);
    } else {
      url_loader_factory_.AddResponse(manage_url.spec(), body.value_or(""));
    }

    bool called = false;
    base::expected<std::string, std::string> result_out;
    service_->GenerateAlias(base::BindOnce(
        [](bool* called_out, decltype(result_out)* out,
           base::expected<std::string, std::string> res) {
          *called_out = true;
          *out = std::move(res);
        },
        &called, &result_out));
    EXPECT_TRUE(base::test::RunUntil([&]() { return called; }));
    return result_out;
  }

  base::expected<std::monostate, std::string> CallUpdateAliasWith(
      const std::string& alias_email,
      const std::optional<std::string>& put_body,
      const std::optional<int>& net_error = std::nullopt,
      const std::optional<std::string>& refresh_body = std::nullopt,
      bool wait_for_update = true) {
    const GURL manage_url =
        EmailAliasesService::GetEmailAliasesServiceBaseURL();
    if (net_error.has_value()) {
      network::URLLoaderCompletionStatus completion(*net_error);
      url_loader_factory_.AddResponse(manage_url, /*head=*/nullptr,
                                      /*content=*/"", completion);
    } else {
      url_loader_factory_.AddResponse(manage_url.spec(), put_body.value_or(""));
    }
    url_loader_factory_.AddResponse(manage_url.Resolve("?status=active").spec(),
                                    refresh_body.value_or("[]"));

    bool called = false;
    base::expected<std::monostate, std::string> result_out;
    service_->UpdateAlias(
        alias_email, /*note=*/std::string("note"),
        base::BindOnce(
            [](bool* called_out, decltype(result_out)* out,
               base::expected<std::monostate, std::string> res) {
              *called_out = true;
              *out = std::move(res);
            },
            &called, &result_out));
    EXPECT_TRUE(base::test::RunUntil([&]() { return called; }));
    if (wait_for_update) {
      EXPECT_TRUE(observer_.WaitForAliasUpdateCount(1));
    }
    return result_out;
  }

  base::expected<std::monostate, std::string> CallDeleteAliasWith(
      const std::string& alias_email,
      const std::optional<std::string>& delete_body,
      const std::optional<int>& net_error = std::nullopt) {
    const GURL manage_url =
        EmailAliasesService::GetEmailAliasesServiceBaseURL();
    if (net_error.has_value()) {
      network::URLLoaderCompletionStatus completion(*net_error);
      auto head = network::mojom::URLResponseHead::New();
      url_loader_factory_.AddResponse(manage_url, std::move(head),
                                      /*content=*/"", completion);
    } else {
      url_loader_factory_.AddResponse(manage_url.spec(),
                                      delete_body.value_or(""));
    }
    url_loader_factory_.AddResponse(manage_url.Resolve("?status=active").spec(),
                                    "[]");

    bool called = false;
    base::expected<std::monostate, std::string> result_out;
    service_->DeleteAlias(
        alias_email, base::BindOnce(
                         [](bool* called_out, decltype(result_out)* out,
                            base::expected<std::monostate, std::string> res) {
                           *called_out = true;
                           *out = std::move(res);
                         },
                         &called, &result_out));
    EXPECT_TRUE(base::test::RunUntil([&]() { return called; }));
    EXPECT_TRUE(observer_.WaitForAliasUpdateCount(1));
    return result_out;
  }
};

TEST_F(EmailAliasesAPITest, GenerateAlias_ReturnsAlias_OnSuccess) {
  const std::string alias_email = "mock-1234@bravealias.com";
  auto result_out = CallGenerateAliasWith(
      std::string("{\"message\":\"created\",\"alias\":\"") + alias_email +
      "\"}");
  ASSERT_TRUE(result_out.has_value());
  EXPECT_EQ(result_out.value(), alias_email);
}

TEST_F(EmailAliasesAPITest, GenerateAlias_ReturnsError_OnBackendErrorMessage) {
  auto result_out =
      CallGenerateAliasWith("{\"message\":\"alias_unavailable\"}");
  ASSERT_FALSE(result_out.has_value());
  EXPECT_EQ(
      result_out.error(),
      l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_ALIAS_NOT_AVAILABLE));
}

TEST_F(EmailAliasesAPITest, GenerateAlias_ReturnsError_OnNoResponseBody) {
  auto result_out = CallGenerateAliasWith(std::nullopt, net::ERR_FAILED);
  ASSERT_FALSE(result_out.has_value());
  EXPECT_EQ(result_out.error(),
            l10n_util::GetStringUTF8(
                IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY));
}

TEST_F(EmailAliasesAPITest, GenerateAlias_ReturnsError_OnInvalidJson) {
  auto result_out = CallGenerateAliasWith("not a json");
  ASSERT_FALSE(result_out.has_value());
  EXPECT_EQ(result_out.error(),
            l10n_util::GetStringUTF8(
                IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY));
}

TEST_F(EmailAliasesAPITest,
       GenerateAlias_ReturnsError_OnUnexpectedSuccessPayload) {
  auto result_out = CallGenerateAliasWith("{\"message\":\"ok_but_no_alias\"}");
  ASSERT_FALSE(result_out.has_value());
  EXPECT_EQ(
      result_out.error(),
      l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_ALIAS_NOT_AVAILABLE));
}

// ============== UpdateAlias tests ==============

TEST_F(EmailAliasesAPITest, UpdateAlias_SucceedsAndRefreshes_OnValidResponse) {
  const std::string alias_email = "alias@example.com";
  auto result_out =
      CallUpdateAliasWith(alias_email,
                          /*put_body=*/"{\"message\":\"updated\"}",
                          /*net_error=*/std::nullopt);
  ASSERT_TRUE(result_out.has_value());
}

TEST_F(EmailAliasesAPITest, UpdateAlias_ReturnsError_OnBackendError) {
  auto result_out =
      CallUpdateAliasWith("alias@example.com",
                          /*put_body=*/"{\"message\":\"backend_error\"}");
  ASSERT_FALSE(result_out.has_value());
}

TEST_F(EmailAliasesAPITest, UpdateAlias_ReturnsError_OnNonUpdatedMessage) {
  auto result_out =
      CallUpdateAliasWith("alias@example.com",
                          /*put_body=*/"{\"message\":\"not_updated\"}");
  ASSERT_FALSE(result_out.has_value());
  EXPECT_TRUE(result_out.error().find("not_updated") != std::string::npos);
}

TEST_F(EmailAliasesAPITest, UpdateAlias_ReturnsError_OnNoResponseBody) {
  auto result_out = CallUpdateAliasWith("alias@example.com",
                                        /*put_body=*/std::nullopt,
                                        /*net_error=*/net::ERR_FAILED);
  ASSERT_FALSE(result_out.has_value());
  EXPECT_EQ(result_out.error(),
            l10n_util::GetStringUTF8(
                IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY));
}

TEST_F(EmailAliasesAPITest, UpdateAlias_ReturnsError_OnInvalidJson) {
  auto result_out = CallUpdateAliasWith("alias@example.com",
                                        /*put_body=*/"not a json");
  ASSERT_FALSE(result_out.has_value());
  EXPECT_EQ(result_out.error(),
            l10n_util::GetStringUTF8(
                IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY));
}
TEST_F(EmailAliasesAPITest, DeleteAlias_SucceedsAndRefreshes_OnValidResponse) {
  auto result_out =
      CallDeleteAliasWith("alias@example.com",
                          /*delete_body=*/"{\"message\":\"deleted\"}");
  ASSERT_TRUE(result_out.has_value());
}

TEST_F(EmailAliasesAPITest, DeleteAlias_ReturnsError_OnBackendError) {
  auto result_out =
      CallDeleteAliasWith("alias@example.com",
                          /*delete_body=*/"{\"message\":\"backend_error\"}");
  ASSERT_FALSE(result_out.has_value());
}

TEST_F(EmailAliasesAPITest, DeleteAlias_ReturnsError_OnNoResponseBody) {
  auto result_out = CallDeleteAliasWith("alias@example.com",
                                        /*delete_body=*/std::nullopt,
                                        /*net_error=*/net::ERR_FAILED);
  ASSERT_FALSE(result_out.has_value());
  EXPECT_EQ(result_out.error(),
            l10n_util::GetStringUTF8(
                IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY));
}

TEST_F(EmailAliasesAPITest, DeleteAlias_ReturnsError_OnInvalidJson) {
  auto result_out = CallDeleteAliasWith("alias@example.com",
                                        /*delete_body=*/"not a json");
  ASSERT_FALSE(result_out.has_value());
  EXPECT_EQ(result_out.error(),
            l10n_util::GetStringUTF8(
                IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY));
}

TEST_F(EmailAliasesAPITest, RefreshAliases_Notifies_OnValidResponse) {
  const std::string alias_email = "alias@example.com";
  auto result_out = CallUpdateAliasWith(
      alias_email,
      /*put_body=*/"{\"message\":\"updated\"}",
      /*net_error=*/std::nullopt,
      /*refresh_body=*/
      std::string("[{\"email\":\"dest@example.com\",\"alias\":\"") +
          alias_email +
          "\",\"created_at\":\"2025-01-01T00:00:00Z\",\"last_used\":\"\","
          "\"status\":\"active\"}]");
  ASSERT_TRUE(result_out.has_value());
  ASSERT_FALSE(observer_.last_aliases.empty());
  EXPECT_EQ(observer_.last_aliases[0]->email, alias_email);
}

TEST_F(EmailAliasesAPITest, RefreshAliases_DoesNotNotify_OnErrorOrInvalidJson) {
  auto result_out =
      CallUpdateAliasWith("alias@example.com",
                          /*put_body=*/"{\"message\":\"updated\"}",
                          /*net_error=*/std::nullopt,
                          /*refresh_body=*/"{\"message\":\"backend_error\"}",
                          /*wait_for_update=*/false);
  ASSERT_TRUE(result_out.has_value());
  EXPECT_EQ(observer_.alias_updates, 0u);
}

TEST_F(EmailAliasesAPITest, ApiFetch_AttachesAuthTokenAndAPIKeyHeaders) {
  // Authenticate to set a non-empty auth token.
  const std::string init_body = "{\"verificationToken\":\"token123\"}";
  const std::string result_body =
      "{\"authToken\":\"auth456\", \"verified\":true, "
      "\"service\":\"email-aliases\"}";
  url_loader_factory_.AddResponse(
      EmailAliasesService::GetAccountsServiceVerifyInitURL().spec(), init_body);
  url_loader_factory_.AddResponse(
      EmailAliasesService::GetAccountsServiceVerifyResultURL().spec(),
      result_body);

  bool auth_called = false;
  service_->RequestAuthentication(
      "test@example.com",
      base::BindOnce(
          [](bool* called_out,
             base::expected<std::monostate, std::string> result) {
            *called_out = true;
            ASSERT_TRUE(result.has_value());
          },
          &auth_called));
  EXPECT_TRUE(base::test::RunUntil([&]() { return auth_called; }));
  // Wait until auth token is set by the session poll response.
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return service_->GetAuthTokenForTesting() == "auth456"; }));

  // Intercept the next manage request to capture headers.
  const GURL manage_url = EmailAliasesService::GetEmailAliasesServiceBaseURL();
  std::string seen_authorization;
  std::string seen_api_key;
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        if (request.url == manage_url) {
          if (auto v = request.headers.GetHeader("Authorization")) {
            seen_authorization = *v;
          }
          if (auto v = request.headers.GetHeader("X-API-key")) {
            seen_api_key = *v;
          }
        }
      }));

  // Provide a successful GenerateAlias response to trigger ApiFetch.
  const std::string alias_email = "mock-1234@bravealias.com";
  url_loader_factory_.AddResponse(
      manage_url.spec(), std::string("{\"message\":\"created\",\"alias\":\"") +
                             alias_email + "\"}");

  auto gen_result = CallGenerateAliasWith(std::nullopt);
  // The helper enqueues the request body separately; here we just ensure it
  // ran. Validate headers captured by the interceptor.
  EXPECT_EQ(seen_authorization, "Bearer auth456");
  EXPECT_EQ(seen_api_key, EmailAliasesService::GetEmailAliasesServiceAPIKey());
}

}  // namespace email_aliases
