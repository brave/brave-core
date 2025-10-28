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
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

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

    if (test_case.endpoint_response) {
      auto as_dict = [](const auto& opt) {
        return opt ? opt->ToValue() : base::Value::Dict();
      };

      const auto& endpoint_expected =
          test_case.endpoint_response->endpoint_expected;
      base::Value value(endpoint_expected.has_value()
                            ? as_dict(endpoint_expected.value())
                            : as_dict(endpoint_expected.error()));

      const auto body = base::WriteJson(value);
      CHECK(body);
      test_url_loader_factory_.AddResponse(
          TestCase::Endpoint::URL().spec(), *body,
          test_case.endpoint_response->http_status_code);
    }

    if constexpr (requires { typename TestCase::MojoExpected; }) {
      base::test::TestFuture<typename TestCase::MojoExpected> future;
      TestCase::Run(test_case, CHECK_DEREF(brave_account_service_.get()),
                    future.GetCallback());
      EXPECT_EQ(future.Take(), test_case.mojo_expected);
    } else {
      TestCase::Run(test_case, pref_service_, task_environment_,
                    *verify_result_timer_);
    }
  }

  bool Encrypt(const std::string& in, std::string* out) {
    *out = in;
    return !CHECK_DEREF(this->GetParam()).fail_encryption;
  }

  bool Decrypt(const std::string& in, std::string* out) {
    *out = in;
    return !CHECK_DEREF(this->GetParam()).fail_decryption;
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
