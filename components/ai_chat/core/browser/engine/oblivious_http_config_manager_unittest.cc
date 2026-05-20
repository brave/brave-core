// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oblivious_http_config_manager.h"

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/containers/flat_map.h"
#include "base/json/values_util.h"
#include "base/memory/raw_ref.h"
#include "base/numerics/clamped_math.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

using ::testing::_;

namespace ai_chat {

namespace {

constexpr char kTestModel[] = "test-model";
constexpr char kTestKeyConfigRaw[] = "raw-hpke-key-bytes";
constexpr char kTestEndpointUrl[] = "https://endpoint.test/inner";

base::Value MakeConfigResponseValue(const std::string& raw_key,
                                    const std::string& endpoint_url) {
  base::DictValue body;
  body.Set("key_config", base::Base64Encode(raw_key));
  body.Set("endpoint_url", endpoint_url);
  return base::Value(std::move(body));
}

}  // namespace

class ObliviousHttpConfigManagerUnitTest : public testing::Test {
 protected:
  void SetUp() override {
    prefs::RegisterProfilePrefs(prefs_.registry());
    manager_ = std::make_unique<ObliviousHttpConfigManager>(
        /*url_loader_factory=*/nullptr, &prefs_);

    auto mock = std::make_unique<
        testing::NiceMock<api_request_helper::MockAPIRequestHelper>>(
        TRAFFIC_ANNOTATION_FOR_TESTS, nullptr);
    mock_helper_ = mock.get();
    manager_->SetAPIRequestHelperForTesting(std::move(mock));

    run_loop_ = std::make_unique<base::RunLoop>();
  }

  void SetUpMock(api_request_helper::APIRequestResult result) {
    EXPECT_CALL(*mock_helper_, Request(_, _, _, _, _, _, _, _))
        .WillOnce([this, result = std::move(result)](
                      const std::string&, const GURL& url, const std::string&,
                      const std::string&,
                      api_request_helper::APIRequestHelper::ResultCallback
                          result_callback,
                      const base::flat_map<std::string, std::string>&,
                      const api_request_helper::APIRequestOptions&,
                      api_request_helper::APIRequestHelper::
                          ResponseConversionCallback) mutable {
          EXPECT_TRUE(url.spec().find(absl::StrFormat(
                          "v1/models/%s/ohttp_config", kTestModel)) !=
                      std::string::npos);
          std::move(result_callback).Run(std::move(result));
          run_loop_->Quit();
          return api_request_helper::APIRequestHelper::Ticket();
        });
  }

  void RequestKeyConfig(const std::string& model_name = kTestModel) {
    manager_->RequestKeyConfig(
        model_name,
        base::BindLambdaForTesting(
            [this](
                std::optional<ObliviousHttpConfigManager::KeyConfigResult> r) {
              key_config_result_ = std::move(r);
            }));
    run_loop_->Run();
  }

  void SeedPrefEntry(const std::string& model_name, bool expired) {
    ScopedDictPrefUpdate update(&prefs_, prefs::kAIChatObliviousHttpKeyConfigs);
    base::DictValue entry;
    entry.Set("key_config", base::Base64Encode(kTestKeyConfigRaw));
    entry.Set("endpoint_url", kTestEndpointUrl);
    base::Time expiry = expired ? base::Time::Now() - base::Seconds(1)
                                : base::Time::Now() + base::Days(3);
    entry.Set("expires_at", base::TimeToValue(expiry));
    update->Set(model_name, std::move(entry));
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<ObliviousHttpConfigManager> manager_;
  raw_ptr<testing::NiceMock<api_request_helper::MockAPIRequestHelper>>
      mock_helper_;
  std::unique_ptr<base::RunLoop> run_loop_;
  std::optional<ObliviousHttpConfigManager::KeyConfigResult> key_config_result_;
};

TEST_F(ObliviousHttpConfigManagerUnitTest, RequestKeyConfig_FetchAndCache) {
  SetUpMock(api_request_helper::APIRequestResult(
      net::HTTP_OK,
      MakeConfigResponseValue(kTestKeyConfigRaw, kTestEndpointUrl), {}, net::OK,
      GURL()));
  RequestKeyConfig();

  ASSERT_TRUE(key_config_result_.has_value());
  EXPECT_EQ(kTestKeyConfigRaw, key_config_result_->key_config);
  EXPECT_EQ(GURL(kTestEndpointUrl), key_config_result_->endpoint_url);

  const base::DictValue* entry =
      prefs_.GetDict(prefs::kAIChatObliviousHttpKeyConfigs)
          .FindDict(kTestModel);
  ASSERT_TRUE(entry);
  const std::string* key_config = entry->FindString("key_config");
  ASSERT_TRUE(key_config);
  EXPECT_EQ(base::Base64Encode(kTestKeyConfigRaw), *key_config);
  const std::string* endpoint_url = entry->FindString("endpoint_url");
  ASSERT_TRUE(endpoint_url);
  EXPECT_EQ(kTestEndpointUrl, *endpoint_url);
  const base::Value* expires_at_val = entry->Find("expires_at");
  ASSERT_TRUE(expires_at_val);
  std::optional<base::Time> expires_at = base::ValueToTime(expires_at_val);
  ASSERT_TRUE(expires_at.has_value());
  EXPECT_GT(*expires_at, base::Time::Now() + base::Days(2));
  EXPECT_LT(*expires_at, base::Time::Now() + base::Days(4));
}

TEST_F(ObliviousHttpConfigManagerUnitTest, RequestKeyConfig_ServesFromCache) {
  SeedPrefEntry(kTestModel, /*expired=*/false);

  EXPECT_CALL(*mock_helper_, Request(_, _, _, _, _, _, _, _)).Times(0);

  manager_->RequestKeyConfig(
      kTestModel,
      base::BindLambdaForTesting(
          [this](std::optional<ObliviousHttpConfigManager::KeyConfigResult> r) {
            key_config_result_ = std::move(r);
          }));

  ASSERT_TRUE(key_config_result_.has_value());
  EXPECT_EQ(kTestKeyConfigRaw, key_config_result_->key_config);
  EXPECT_EQ(GURL(kTestEndpointUrl), key_config_result_->endpoint_url);
}

TEST_F(ObliviousHttpConfigManagerUnitTest,
       DeleteExpiredKeyConfigs_RemovesExpiredKeepsValid) {
  SeedPrefEntry("expired-model-1", /*expired=*/true);
  SeedPrefEntry("expired-model-2", /*expired=*/true);
  SeedPrefEntry("valid-model", /*expired=*/false);

  ObliviousHttpConfigManager::DeleteExpiredKeyConfigs(&prefs_);

  const base::DictValue& all =
      prefs_.GetDict(prefs::kAIChatObliviousHttpKeyConfigs);
  EXPECT_FALSE(all.FindDict("expired-model-1"));
  EXPECT_FALSE(all.FindDict("expired-model-2"));
  EXPECT_TRUE(all.FindDict("valid-model"));
}

TEST_F(ObliviousHttpConfigManagerUnitTest, ClearKeyConfig_ClearsEntry) {
  SeedPrefEntry(kTestModel, /*expired=*/false);
  manager_->ClearKeyConfig(kTestModel);

  EXPECT_FALSE(prefs_.GetDict(prefs::kAIChatObliviousHttpKeyConfigs)
                   .FindDict(kTestModel));
}

TEST_F(ObliviousHttpConfigManagerUnitTest, ClearKeyConfig_ForcesRefetch) {
  SeedPrefEntry(kTestModel, /*expired=*/false);

  task_environment_.FastForwardBy(base::Days(4));

  SetUpMock(api_request_helper::APIRequestResult(
      net::HTTP_OK,
      MakeConfigResponseValue(kTestKeyConfigRaw, kTestEndpointUrl), {}, net::OK,
      GURL()));
  RequestKeyConfig();

  ASSERT_TRUE(key_config_result_.has_value());
  EXPECT_EQ(kTestKeyConfigRaw, key_config_result_->key_config);
}

TEST_F(ObliviousHttpConfigManagerUnitTest, RequestKeyConfig_HttpError) {
  SetUpMock(api_request_helper::APIRequestResult(
      net::HTTP_INTERNAL_SERVER_ERROR, base::Value(), {}, net::OK, GURL()));
  RequestKeyConfig();

  EXPECT_FALSE(key_config_result_.has_value());
}

TEST_F(ObliviousHttpConfigManagerUnitTest,
       RequestKeyConfig_MissingKeyConfigField) {
  base::DictValue body;
  body.Set("endpoint_url", kTestEndpointUrl);
  SetUpMock(api_request_helper::APIRequestResult(
      net::HTTP_OK, base::Value(std::move(body)), {}, net::OK, GURL()));
  RequestKeyConfig();

  EXPECT_FALSE(key_config_result_.has_value());
}

TEST_F(ObliviousHttpConfigManagerUnitTest,
       RequestKeyConfig_NonHttpsEndpointUrl) {
  SetUpMock(api_request_helper::APIRequestResult(
      net::HTTP_OK,
      MakeConfigResponseValue(kTestKeyConfigRaw, "http://endpoint.test/inner"),
      {}, net::OK, GURL()));
  RequestKeyConfig();

  EXPECT_FALSE(key_config_result_.has_value());
}

}  // namespace ai_chat
