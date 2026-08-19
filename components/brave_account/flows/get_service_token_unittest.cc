/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/get_service_token.h"

#include <optional>
#include <string>

#include "base/base64.h"
#include "base/functional/callback.h"
#include "base/json/values_util.h"
#include "base/no_destructor.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/service_token.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/get_service_token.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::ServiceToken;

struct GetServiceTokenTestCase {
  using Endpoint = ServiceToken;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::GetServiceTokenResultPtr,
                                      mojom::GetServiceTokenErrorPtr>;

  static void Run(const GetServiceTokenTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    AccountStatePrefs(pref_service)
        .SetLoggedIn(kEmailAddress, EncryptedAuthenticationToken());
    ScopedDictPrefUpdate(&pref_service, prefs::kBraveAccountState)
        ->Set(prefs::keys::kServiceTokens,
              std::move(test_case.service_tokens_dict).Run(base::Time::Now()));

    task_environment.FastForwardBy(test_case.time_advance);

    std::string expected_service_token =
        test_case.mojo_expected.has_value()
            ? test_case.mojo_expected.value()->serviceToken
            : "";

    authentication->GetServiceToken(
        mojom::Service::kEmailAliases,
        // |callback| resolves the TestFuture in BraveAccountServiceTest with
        // the result. The Then() callback runs immediately after, before the
        // test verifies the future's value.
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, std::string expected_service_token) {
              if (!expected_service_token.empty()) {
                const auto* service_tokens =
                    pref_service->GetDict(prefs::kBraveAccountState)
                        .FindDict(prefs::keys::kServiceTokens);
                ASSERT_TRUE(service_tokens);
                const auto* email_aliases =
                    service_tokens->FindDict("email-aliases");
                ASSERT_TRUE(email_aliases);
                const auto* service_token =
                    email_aliases->FindString(prefs::keys::kServiceToken);
                ASSERT_TRUE(service_token);
                EXPECT_EQ(*service_token,
                          base::Base64Encode(expected_service_token));
              }
            },
            base::Unretained(&pref_service),
            std::move(expected_service_token))));
  }

  std::string test_name;
  // |service_tokens_dict| is a callback instead of a plain base::DictValue
  // so that test cases can use the current mock time (passed as parameter)
  // when constructing the dictionary. This is necessary for cache expiration
  // tests that need to set timestamps relative to when the test runs.
  mutable base::OnceCallback<base::DictValue(base::Time)> service_tokens_dict;
  bool fail_decryption;
  bool fail_encryption;
  base::TimeDelta time_advance;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const GetServiceTokenTestCase* GetServiceTokenCacheHit() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_cache_hit",
      .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
        return base::DictValue().Set(
            "email-aliases",
            base::DictValue()
                .Set(prefs::keys::kServiceToken,
                     base::Base64Encode("cached_service_token"))
                .Set(prefs::keys::kLastFetched, base::TimeToValue(mock_now)));
      }),
      .fail_decryption = {},    // not used
      .fail_encryption = {},    // not used
      .time_advance = {},       // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          mojom::GetServiceTokenResult::New("cached_service_token"),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase*
GetServiceTokenAuthenticationTokenDecryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_authentication_token_decryption_failed",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = true,
      .fail_encryption = {},    // not used
      .time_advance = {},       // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::GetServiceTokenError::NewClientError(
              mojom::GetServiceTokenClientError::New(
                  mojom::GetServiceTokenClientErrorCode::
                      kAuthenticationTokenDecryptionFailed))),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenBodyMissingOrFailedToParse() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_body_missing_or_failed_to_parse",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .time_advance = {},     // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}},
      .mojo_expected =
          base::unexpected(mojom::GetServiceTokenError::NewServerError(
              mojom::GetServiceTokenServerError::New(
                  net::HTTP_INTERNAL_SERVER_ERROR,
                  mojom::GetServiceTokenServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenErrorCodeIsNull() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_error_code_is_null",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .time_advance = {},     // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_BAD_REQUEST,
                             .body = base::unexpected([] {
                               ServiceToken::Response::ErrorBody body;
                               body.code = base::Value();
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::GetServiceTokenError::NewServerError(
              mojom::GetServiceTokenServerError::New(
                  net::HTTP_BAD_REQUEST,
                  mojom::GetServiceTokenServerErrorCode::kNull))),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenUnknownErrorCode() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_unknown_error_code",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .time_advance = {},     // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_TOO_EARLY,
                             .body = base::unexpected([] {
                               ServiceToken::Response::ErrorBody body;
                               body.code = base::Value(42);
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::GetServiceTokenError::NewServerError(
              mojom::GetServiceTokenServerError::New(
                  net::HTTP_TOO_EARLY,
                  mojom::GetServiceTokenServerErrorCode::kUnknown))),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenKnownErrorCode() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_known_error_code",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .time_advance = {},     // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_BAD_REQUEST,
                             .body = base::unexpected([] {
                               ServiceToken::Response::ErrorBody body;
                               body.code = base::Value(13006);
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::GetServiceTokenError::NewServerError(
              mojom::GetServiceTokenServerError::New(
                  net::HTTP_BAD_REQUEST, mojom::GetServiceTokenServerErrorCode::
                                             kEmailDomainNotSupported))),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenEmpty() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_service_token_empty",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .time_advance = {},     // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "";
                                   return body;
                                 }()}},
      .mojo_expected =
          base::unexpected(mojom::GetServiceTokenError::NewServerError(
              mojom::GetServiceTokenServerError::New(
                  net::HTTP_OK,
                  mojom::GetServiceTokenServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenEncryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_service_token_encryption_failed",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = false,
      .fail_encryption = true,
      .time_advance = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "fetched_service_token";
                                   return body;
                                 }()}},
      .mojo_expected =
          base::unexpected(mojom::GetServiceTokenError::NewClientError(
              mojom::GetServiceTokenClientError::New(
                  mojom::GetServiceTokenClientErrorCode::
                      kServiceTokenEncryptionFailed))),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenSuccess() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_success",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = false,
      .fail_encryption = false,
      .time_advance = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "fetched_service_token";
                                   return body;
                                 }()}},
      .mojo_expected =
          mojom::GetServiceTokenResult::New("fetched_service_token"),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceDictMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_service_dict_missing",
      .service_tokens_dict =
          base::BindOnce([](base::Time) { return base::DictValue(); }),
      .fail_decryption = false,
      .fail_encryption = false,
      .time_advance = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "fetched_service_token";
                                   return body;
                                 }()}},
      .mojo_expected =
          mojom::GetServiceTokenResult::New("fetched_service_token"),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_cache_service_token_missing",
      .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
        return base::DictValue().Set(
            "email-aliases",
            base::DictValue().Set(prefs::keys::kLastFetched,
                                  base::TimeToValue(mock_now)));
      }),
      .fail_decryption = false,
      .fail_encryption = false,
      .time_advance = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "fetched_service_token";
                                   return body;
                                 }()}},
      .mojo_expected =
          mojom::GetServiceTokenResult::New("fetched_service_token"),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenLastFetchedMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_cache_last_fetched_missing",
      .service_tokens_dict = base::BindOnce([](base::Time) {
        return base::DictValue().Set(
            "email-aliases",
            base::DictValue().Set(prefs::keys::kServiceToken,
                                  base::Base64Encode("cached_service_token")));
      }),
      .fail_decryption = false,
      .fail_encryption = false,
      .time_advance = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "fetched_service_token";
                                   return body;
                                 }()}},
      .mojo_expected =
          mojom::GetServiceTokenResult::New("fetched_service_token"),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenLastFetchedInvalid() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_cache_last_fetched_invalid",
      .service_tokens_dict = base::BindOnce([](base::Time) {
        return base::DictValue().Set(
            "email-aliases",
            base::DictValue()
                .Set(prefs::keys::kServiceToken,
                     base::Base64Encode("cached_service_token"))
                .Set(prefs::keys::kLastFetched, "invalid-time-format"));
      }),
      .fail_decryption = false,
      .fail_encryption = false,
      .time_advance = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "fetched_service_token";
                                   return body;
                                 }()}},
      .mojo_expected =
          mojom::GetServiceTokenResult::New("fetched_service_token"),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenCacheExpired() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_cache_expired",
      .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
        return base::DictValue().Set(
            "email-aliases",
            base::DictValue()
                .Set(prefs::keys::kServiceToken,
                     base::Base64Encode("cached_service_token"))
                .Set(prefs::keys::kLastFetched, base::TimeToValue(mock_now)));
      }),
      .fail_decryption = false,
      .fail_encryption = false,
      // Advance time after setting cache to make it expired.
      .time_advance = kServiceTokenMaxAge + base::Minutes(1),
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "fetched_service_token";
                                   return body;
                                 }()}},
      .mojo_expected =
          mojom::GetServiceTokenResult::New("fetched_service_token"),
  });
  return kTestCase.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenDecryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase> kTestCase({
      .test_name = "get_service_token_cache_service_token_decryption_failed",
      .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
        return base::DictValue().Set(
            "email-aliases",
            base::DictValue()
                .Set(prefs::keys::kServiceToken, "!!!invalid-base64!!!")
                .Set(prefs::keys::kLastFetched, base::TimeToValue(mock_now)));
      }),
      .fail_decryption = false,
      .fail_encryption = false,
      .time_advance = base::TimeDelta(),
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   ServiceToken::Response::SuccessBody body;
                                   body.auth_token = "fetched_service_token";
                                   return body;
                                 }()}},
      .mojo_expected =
          mojom::GetServiceTokenResult::New("fetched_service_token"),
  });
  return kTestCase.get();
}

using BraveAccountServiceGetServiceTokenTest =
    BraveAccountServiceTest<GetServiceTokenTestCase>;

}  // namespace

TEST_P(BraveAccountServiceGetServiceTokenTest, GetServiceToken) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceGetServiceTokenTest,
    testing::Values(GetServiceTokenCacheHit(),
                    GetServiceTokenAuthenticationTokenDecryptionFailed(),
                    GetServiceTokenBodyMissingOrFailedToParse(),
                    GetServiceTokenErrorCodeIsNull(),
                    GetServiceTokenUnknownErrorCode(),
                    GetServiceTokenKnownErrorCode(),
                    GetServiceTokenServiceTokenEmpty(),
                    GetServiceTokenServiceTokenEncryptionFailed(),
                    GetServiceTokenSuccess(),
                    GetServiceTokenServiceDictMissing(),
                    GetServiceTokenServiceTokenMissing(),
                    GetServiceTokenLastFetchedMissing(),
                    GetServiceTokenLastFetchedInvalid(),
                    GetServiceTokenCacheExpired(),
                    GetServiceTokenServiceTokenDecryptionFailed()),
    BraveAccountServiceGetServiceTokenTest::kNameGenerator);

}  // namespace brave_account
