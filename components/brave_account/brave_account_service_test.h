/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_

#include <memory>
#include <string>

#include "base/base64.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_encryption.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/endpoint_client/test_support.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/prefs.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

inline constexpr char kAuthenticationToken[] = "authentication_token";
inline constexpr char kEmailAddress[] = "email@address.com";
inline constexpr char kLoginToken[] = "login_token";
inline constexpr char kVerificationToken[] = "verification_token";

inline const std::string& EncryptedAuthenticationToken() {
  static const base::NoDestructor<std::string> kEncryptedAuthenticationToken(
      base::Base64Encode(kAuthenticationToken));
  return *kEncryptedAuthenticationToken;
}

inline const std::string& EncryptedLoginToken() {
  static const base::NoDestructor<std::string> kEncryptedLoginToken(
      base::Base64Encode(kLoginToken));
  return *kEncryptedLoginToken;
}

inline const std::string& EncryptedVerificationToken() {
  static const base::NoDestructor<std::string> kEncryptedVerificationToken(
      base::Base64Encode(kVerificationToken));
  return *kEncryptedVerificationToken;
}

struct AuthenticationObserverTestCase;
struct AuthValidateTestCase;
struct CancelVerificationTestCase;
struct ChangePasswordPasswordFinalizeTestCase;
struct ChangePasswordPasswordInitTestCase;
struct ChangePasswordVerifyCompleteTestCase;
struct ChangePasswordVerifyInitTestCase;
struct GetServiceTokenTestCase;
struct LoginFinalizeTestCase;
struct LoginInitializeTestCase;
struct LogOutTestCase;
struct RegisterFinalizeTestCase;
struct RegisterInitializeTestCase;
struct RegisterVerifyTestCase;
struct ResendVerificationEmailTestCase;
struct ResetPasswordPasswordFinalizeTestCase;
struct ResetPasswordPasswordInitTestCase;
struct ResetPasswordVerifyCompleteTestCase;
struct ResetPasswordVerifyInitTestCase;

template <typename TestCase>
class BraveAccountServiceTest : public testing::TestWithParam<const TestCase*> {
 public:
  static constexpr auto kNameGenerator = [](const auto& info) {
    return CHECK_DEREF(info.param).test_name;
  };

 protected:
  void SetUp() override {
    prefs::RegisterPrefs(pref_service_.registry());
    BraveAccountEncryption::SetOSCryptCallbacksForTesting(
        base::BindRepeating(&BraveAccountServiceTest::Encrypt,
                            base::Unretained(this)),
        base::BindRepeating(&BraveAccountServiceTest::Decrypt,
                            base::Unretained(this)));
    os_crypt_async_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);
    brave_account_service_ = std::make_unique<BraveAccountService>(
        &pref_service_, test_url_loader_factory_.GetSafeWeakWrapper(),
        os_crypt_async_.get());
    brave_account_service_->BindInterface(
        authentication_.BindNewPipeAndPassReceiver());
  }

  void TearDown() override {
    BraveAccountEncryption::SetOSCryptCallbacksForTesting(base::NullCallback(),
                                                          base::NullCallback());
  }

  void RunTestCase() {
    const auto& test_case = CHECK_DEREF(this->GetParam());

    if constexpr (requires { typename TestCase::Endpoint; }) {
      if (test_case.endpoint_response) {
        endpoint_client::MockResponseFor<typename TestCase::Endpoint>(
            test_url_loader_factory_, *test_case.endpoint_response);
      }
    }

    if constexpr (
        std::is_same_v<TestCase, ChangePasswordPasswordFinalizeTestCase> ||
        std::is_same_v<TestCase, ChangePasswordPasswordInitTestCase> ||
        std::is_same_v<TestCase, ChangePasswordVerifyCompleteTestCase> ||
        std::is_same_v<TestCase, ChangePasswordVerifyInitTestCase> ||
        std::is_same_v<TestCase, GetServiceTokenTestCase> ||
        std::is_same_v<TestCase, LoginFinalizeTestCase> ||
        std::is_same_v<TestCase, LoginInitializeTestCase> ||
        std::is_same_v<TestCase, RegisterFinalizeTestCase> ||
        std::is_same_v<TestCase, RegisterInitializeTestCase> ||
        std::is_same_v<TestCase, RegisterVerifyTestCase> ||
        std::is_same_v<TestCase, ResendVerificationEmailTestCase> ||
        std::is_same_v<TestCase, ResetPasswordPasswordFinalizeTestCase> ||
        std::is_same_v<TestCase, ResetPasswordPasswordInitTestCase> ||
        std::is_same_v<TestCase, ResetPasswordVerifyCompleteTestCase> ||
        std::is_same_v<TestCase, ResetPasswordVerifyInitTestCase>) {
      base::test::TestFuture<typename TestCase::MojoExpected> future;
      TestCase::Run(test_case, pref_service_, task_environment_,
                    authentication_, future.GetCallback());
      EXPECT_EQ(future.Take(), test_case.mojo_expected);
    } else if constexpr (std::is_same_v<TestCase, AuthValidateTestCase>) {
      TestCase::Run(test_case, pref_service_, task_environment_,
                    CHECK_DEREF(brave_account_service_.get()));
    } else if constexpr (std::is_same_v<TestCase,
                                        AuthenticationObserverTestCase> ||
                         std::is_same_v<TestCase, CancelVerificationTestCase> ||
                         std::is_same_v<TestCase, LogOutTestCase>) {
      TestCase::Run(test_case, pref_service_, authentication_);
    } else {
      static_assert(false);
    }
  }

  bool Encrypt(const std::string& in, std::string* out) {
    *out = in;
    if constexpr (requires(TestCase test_case) { test_case.fail_encryption; }) {
      return !CHECK_DEREF(this->GetParam()).fail_encryption;
    }
    return true;
  }

  bool Decrypt(const std::string& in, std::string* out) {
    *out = in;
    if constexpr (requires(TestCase test_case) { test_case.fail_decryption; }) {
      return !CHECK_DEREF(this->GetParam()).fail_decryption;
    }
    return true;
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::test::ScopedFeatureList scoped_feature_list_{
      features::BraveAccountFeatureForTesting()};
  TestingPrefServiceSimple pref_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_;
  std::unique_ptr<BraveAccountService> brave_account_service_;
  mojo::Remote<mojom::Authentication> authentication_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_
