/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_

#include <memory>
#include <string>
#include <utility>

#include "base/check_deref.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_account/prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

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
    auto mock_api_request_helper =
        std::make_unique<api_request_helper::MockAPIRequestHelper>(
            TRAFFIC_ANNOTATION_FOR_TESTS,
            test_url_loader_factory_.GetSafeWeakWrapper());
    mock_api_request_helper_ = mock_api_request_helper.get();
    brave_account_service_ = base::WrapUnique(new BraveAccountService(
        &pref_service_, std::move(mock_api_request_helper),
        base::BindRepeating(&BraveAccountServiceTest::Encrypt,
                            base::Unretained(this)),
        base::BindRepeating(&BraveAccountServiceTest::Decrypt,
                            base::Unretained(this))));
  }

  void RunTestCase() {
    const auto& test_case = CHECK_DEREF(this->GetParam());

    // may be skipped if the flow ends before invoking APIRequestHelper
    EXPECT_CALL(*mock_api_request_helper_, Request)
        .Times(testing::AtMost(1))
        .WillOnce([&](const auto&, const auto&, const auto&, const auto&,
                      api_request_helper::MockAPIRequestHelper::ResultCallback
                          callback,
                      const auto&, const auto&, auto) {
          auto as_dict = [](const auto& opt) {
            return opt ? opt->ToValue() : base::Value::Dict();
          };

          base::Value value(test_case.endpoint_expected.has_value()
                                ? as_dict(test_case.endpoint_expected.value())
                                : as_dict(test_case.endpoint_expected.error()));

          api_request_helper::APIRequestResult result(
              test_case.http_status_code, std::move(value),
              base::flat_map<std::string, std::string>(), net::OK, GURL());

          std::move(callback).Run(std::move(result));

          return api_request_helper::MockAPIRequestHelper::Ticket();
        });

    if constexpr (requires { typename TestCase::MojoExpected; }) {
      base::test::TestFuture<typename TestCase::MojoExpected> future;
      TestCase::Run(test_case, CHECK_DEREF(brave_account_service_.get()),
                    future.GetCallback());
      EXPECT_EQ(future.Take(), test_case.mojo_expected);
    } else {
      TestCase::Run(test_case, pref_service_, task_environment_);
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
  raw_ptr<api_request_helper::MockAPIRequestHelper> mock_api_request_helper_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_TEST_H_
