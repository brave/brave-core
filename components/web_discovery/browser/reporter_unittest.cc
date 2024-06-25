/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/reporter.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/test/task_environment.h"
#include "brave/components/web_discovery/browser/regex_util.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/browser/util.h"
#include "brave/components/web_discovery/browser/wdp_service.h"
#include "components/prefs/testing_pref_service.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

namespace {
constexpr char kTestPubKey[] =
    "BECQDFoOR0DE3wLaDidGAC/2Mpgjasf9QgJDGGLTkTdll+pW2S/"
    "RgX0pkFyDjQZc6efyX3RGQKJ2cq8HOB8vZOo=";
}

class WebDiscoveryReporterTest : public testing::Test {
 public:
  WebDiscoveryReporterTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~WebDiscoveryReporterTest() override = default;

  // testing::Test:
  void SetUp() override {
    WDPService::RegisterProfilePrefs(profile_prefs_.registry());
    auto server_config = std::make_unique<ServerConfig>();

    auto action_config = std::make_unique<SourceMapActionConfig>();
    action_config->keys.push_back("q->url");
    action_config->period = 24;
    action_config->limit = 3;
    server_config->source_map_actions["query"] = std::move(action_config);

    for (size_t i = 0; i < 3; i++) {
      base::Time date = base::Time::Now() + base::Days(i);
      server_config->pub_keys[FormatServerDate(date)] = kTestPubKey;
    }

    server_config_loader_ = std::make_unique<ServerConfigLoader>(
        nullptr, base::FilePath(), nullptr, base::DoNothing(),
        base::DoNothing());
    server_config_loader_->SetLastServerConfigForTesting(
        std::move(server_config));

    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &WebDiscoveryReporterTest::HandleRequest, base::Unretained(this)));

    AddCredentialForToday();
    SetupReporter();
  }

 protected:
  class TestCredentialSigner : public CredentialSigner {
   public:
    bool CredentialExistsForToday() override {
      std::string today = FormatServerDate(base::Time::Now());
      return allowed_credentials_.contains(today);
    }

    bool Sign(std::vector<const uint8_t> msg,
              std::vector<const uint8_t> basename,
              SignCallback callback) override {
      if (CredentialExistsForToday()) {
        std::vector<const uint8_t> dummy_signature(
            {static_cast<uint8_t>(sign_count_ + 1)});
        std::move(callback).Run(std::move(dummy_signature));
        sign_count_++;
      } else {
        std::move(callback).Run(std::nullopt);
      }
      return true;
    }

    size_t sign_count_ = 0;
    base::flat_set<std::string> allowed_credentials_;
  };

  void SetupReporter() {
    reporter_ = std::make_unique<Reporter>(
        &profile_prefs_, shared_url_loader_factory_.get(), &credential_signer_,
        &regex_util_, server_config_loader_.get());
  }

  void AddCredentialForToday() {
    std::string today = FormatServerDate(base::Time::Now());
    credential_signer_.allowed_credentials_.insert(today);
  }

  base::Value::Dict GenerateTestPayload() {
    base::Value::Dict payload;
    base::Value::Dict inner_payload;
    inner_payload.Set("q", "test query");
    payload.Set("payload", std::move(inner_payload));
    payload.Set("action", "query");
    return payload;
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<Reporter> reporter_;
  TestCredentialSigner credential_signer_;
  size_t report_requests_made_ = 0;
  net::HttpStatusCode submit_status_code_ = net::HTTP_OK;

 private:
  void HandleRequest(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();

    EXPECT_EQ(request.url.spec(), GetAnonymousHPNHost() + "/");
    EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
    std::string key_date, encryption, version;
    request.headers.GetHeader("Key-Date", &key_date);
    request.headers.GetHeader("Encryption", &encryption);
    request.headers.GetHeader(kVersionHeader, &version);
    EXPECT_EQ(key_date, FormatServerDate(base::Time::Now()));
    auto decoded_pubkey_and_iv = base::Base64Decode(encryption);
    ASSERT_TRUE(decoded_pubkey_and_iv);
    EXPECT_EQ(decoded_pubkey_and_iv->size(), 78u);
    EXPECT_EQ(version, base::NumberToString(kCurrentVersion));

    std::string response;
    const auto* elements = request.request_body->elements();
    ASSERT_EQ(elements->size(), 1u);
    ASSERT_EQ(elements->at(0).type(), network::DataElement::Tag::kBytes);
    auto body = elements->at(0).As<network::DataElementBytes>().bytes();
    EXPECT_FALSE(body.empty());

    url_loader_factory_.AddResponse(request.url.spec(), "",
                                    submit_status_code_);
    report_requests_made_++;
  }

  std::unique_ptr<ServerConfigLoader> server_config_loader_;
  RegexUtil regex_util_;
  TestingPrefServiceSimple profile_prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(WebDiscoveryReporterTest, BasicReport) {
  reporter_->ScheduleSend(GenerateTestPayload());
  reporter_->ScheduleSend(GenerateTestPayload());
  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);

  task_environment_.FastForwardBy(base::Seconds(30));

  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);

  task_environment_.FastForwardBy(base::Seconds(60));

  EXPECT_EQ(report_requests_made_, 1u);
  EXPECT_EQ(credential_signer_.sign_count_, 1u);

  task_environment_.FastForwardBy(base::Seconds(80));

  EXPECT_EQ(report_requests_made_, 2u);
  EXPECT_EQ(credential_signer_.sign_count_, 2u);
  report_requests_made_ = 0;
  credential_signer_.sign_count_ = 0;

  task_environment_.FastForwardBy(base::Minutes(5));

  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);
}

TEST_F(WebDiscoveryReporterTest, LoadReportFromStorage) {
  reporter_->ScheduleSend(GenerateTestPayload());
  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);

  SetupReporter();

  task_environment_.FastForwardBy(base::Seconds(30));

  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);

  task_environment_.FastForwardBy(base::Seconds(60));

  EXPECT_EQ(report_requests_made_, 1u);
  EXPECT_EQ(credential_signer_.sign_count_, 1u);
}

TEST_F(WebDiscoveryReporterTest, CredentialUnavailableRetry) {
  task_environment_.FastForwardBy(base::Days(1));

  reporter_->ScheduleSend(GenerateTestPayload());
  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);

  task_environment_.FastForwardBy(base::Seconds(150));
  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);

  AddCredentialForToday();
  task_environment_.FastForwardBy(base::Seconds(120));

  EXPECT_EQ(report_requests_made_, 1u);
  EXPECT_EQ(credential_signer_.sign_count_, 1u);
  report_requests_made_ = 0;
  credential_signer_.sign_count_ = 0;

  task_environment_.FastForwardBy(base::Minutes(5));

  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);
}

TEST_F(WebDiscoveryReporterTest, ServerUnavailableRetry) {
  submit_status_code_ = net::HTTP_INTERNAL_SERVER_ERROR;
  reporter_->ScheduleSend(GenerateTestPayload());

  task_environment_.FastForwardBy(base::Seconds(80));
  EXPECT_GE(report_requests_made_, 1u);
  EXPECT_GE(credential_signer_.sign_count_, 1u);

  size_t prev_report_requests_made = report_requests_made_;
  size_t prev_sign_count = credential_signer_.sign_count_;
  task_environment_.FastForwardBy(base::Seconds(100));

  EXPECT_GT(report_requests_made_, prev_report_requests_made);
  EXPECT_GT(credential_signer_.sign_count_, prev_sign_count);
  report_requests_made_ = 0;
  credential_signer_.sign_count_ = 0;

  submit_status_code_ = net::HTTP_OK;
  task_environment_.FastForwardBy(base::Seconds(100));

  EXPECT_EQ(report_requests_made_, 1u);
  EXPECT_EQ(credential_signer_.sign_count_, 1u);
  report_requests_made_ = 0;
  credential_signer_.sign_count_ = 0;

  task_environment_.FastForwardBy(base::Minutes(5));

  EXPECT_EQ(report_requests_made_, 0u);
  EXPECT_EQ(credential_signer_.sign_count_, 0u);
}

}  // namespace web_discovery
