/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_

#include <memory>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_account/prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

struct RegisterInitializeTestCase;
struct RegisterFinalizeTestCase;
struct VerifyResultTestCase;
struct CancelRegistrationTestCase;
struct LogOutTestCase;

template <typename TestCase>
class BraveAccountServiceTest : public testing::TestWithParam<const TestCase*> {
 public:
  static constexpr auto kNameGenerator = [](const auto& info) {
    return CHECK_DEREF(info.param).test_name;
  };

 protected:
  void SetUp() override {
    prefs::RegisterPrefs(pref_service_.registry());
    auto verify_result_timer = std::make_unique<base::OneShotTimer>();
    verify_result_timer_ = verify_result_timer.get();
    brave_account_service_ = base::WrapUnique(new BraveAccountService(
        &pref_service_, test_url_loader_factory_.GetSafeWeakWrapper(),
        base::BindRepeating(&BraveAccountServiceTest::Encrypt,
                            base::Unretained(this)),
        base::BindRepeating(&BraveAccountServiceTest::Decrypt,
                            base::Unretained(this)),
        std::move(verify_result_timer)));
  }

  void RunTestCase() {
    const auto& test_case = CHECK_DEREF(this->GetParam());

    if constexpr (requires { typename TestCase::Endpoint; }) {
      if (test_case.endpoint_response) {
        const auto& endpoint_response_status_code =
            test_case.endpoint_response->status_code;
        CHECK(endpoint_response_status_code);

        const auto& endpoint_response_body = test_case.endpoint_response->body;
        base::Value value(endpoint_response_body
                              ? endpoint_response_body->has_value()
                                    ? endpoint_response_body->value().ToValue()
                                    : endpoint_response_body->error().ToValue()
                              : base::Value::Dict());
        const auto body = base::WriteJson(value);
        CHECK(body);

        test_url_loader_factory_.AddResponse(
            TestCase::Endpoint::URL().spec(), *body,
            static_cast<net::HttpStatusCode>(*endpoint_response_status_code));
      }
    }

    if constexpr (std::is_same_v<TestCase, RegisterInitializeTestCase> ||
                  std::is_same_v<TestCase, RegisterFinalizeTestCase>) {
      base::test::TestFuture<typename TestCase::MojoExpected> future;
      TestCase::Run(test_case, CHECK_DEREF(brave_account_service_.get()),
                    future.GetCallback());
      EXPECT_EQ(future.Take(), test_case.mojo_expected);
    } else if constexpr (std::is_same_v<TestCase, VerifyResultTestCase>) {
      TestCase::Run(test_case, pref_service_, task_environment_,
                    *verify_result_timer_);
    } else if constexpr (std::is_same_v<TestCase, CancelRegistrationTestCase> ||
                         std::is_same_v<TestCase, LogOutTestCase>) {
      TestCase::Run(test_case, pref_service_,
                    CHECK_DEREF(brave_account_service_.get()));
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
  base::test::ScopedFeatureList scoped_feature_list_{features::kBraveAccount};
  TestingPrefServiceSimple pref_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  std::unique_ptr<BraveAccountService> brave_account_service_;
  raw_ptr<base::OneShotTimer> verify_result_timer_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_
