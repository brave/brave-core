/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"

#include <memory>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace webcompat_reporter {

class WebcompatReportUploaderUnitTest : public testing::Test {
 public:
  WebcompatReportUploaderUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    webcompat_report_uploader_ =
        std::make_unique<WebcompatReportUploader>(shared_url_loader_factory_);
  }

  using OnRequestPayloadCallback =
      base::RepeatingCallback<void(const std::string& request_payload)>;

  void SetInterceptor(
      OnRequestPayloadCallback* request_payload_callback = nullptr) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, request_payload_callback](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          if (request_payload_callback) {
            std::string request_string(request.request_body->elements()
                                           ->at(0)
                                           .As<network::DataElementBytes>()
                                           .AsStringPiece());
            request_payload_callback->Run(request_string);
          }
        }));
  }

  WebcompatReportUploader* GetWebcompatUploader() {
    return webcompat_report_uploader_.get();
  }

  void TestReportGeneration(const Report& report, const std::string& content) {
    auto content_vals = base::test::ParseJson(content);
    EXPECT_TRUE(content_vals.is_dict());
    auto& content_dict_vals = content_vals.GetDict();

    base::RunLoop run_loop;
    auto request_payload_check_callback =
        base::BindLambdaForTesting([&](const std::string& request_payload) {
          auto request_vals = base::test::ParseJson(request_payload);
          EXPECT_TRUE(request_vals.is_dict());

          auto& request_dict_vals = request_vals.GetDict();

          for (base::Value::Dict::iterator it = content_dict_vals.begin();
               it != content_dict_vals.end(); ++it) {
            EXPECT_NE(request_dict_vals.Find(it->first), nullptr);
            auto* request_val = request_dict_vals.Find(it->first);
            if (request_val->is_bool()) {
              EXPECT_EQ(*request_val, it->second == "true");
            } else {
              EXPECT_EQ(*request_val, it->second);
            }
          }
          run_loop.Quit();
        });

    SetInterceptor(&request_payload_check_callback);
    GetWebcompatUploader()->SubmitReport(report);
    run_loop.Run();
  }

 protected:
  std::unique_ptr<WebcompatReportUploader> webcompat_report_uploader_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_F(WebcompatReportUploaderUnitTest, GenerateReport) {
  Report report;
  TestReportGeneration(report,
                       base::StringPrintf(R"({"api_key":"%s"})",
                                          brave_stats::GetAPIKey().c_str()));

  report.channel = "dev";
  report.brave_version = "1.231.45";
  report.shields_enabled = "true";
  report.language_farbling = "true";
  report.brave_vpn_connected = "true";
  report.ad_block_setting = "ad_block_setting";
  report.fp_block_setting = "fp_block_setting";
  report.ad_block_list_names = "ad_block_list_names";
  report.languages = "languages";
  base::Value::Dict test_dict;
  test_dict.Set("key1", "val1");
  test_dict.Set("key1", "val1");
  report.ad_block_components = base::Value(test_dict.Clone());
  report.details = "details";
  report.contact = "contact";
  report.report_url = GURL("https://abc.url/p1/p2");

  std::string test_dict_string;
  auto serialize_success =
      base::JSONWriter::Write(test_dict, &test_dict_string);
  EXPECT_TRUE(serialize_success);

  TestReportGeneration(
      report,
      base::StringPrintf(
          R"({
  "adBlockComponentsInfo": %s,
  "adBlockLists": "%s",
  "adBlockSetting": "%s",
  "additionalDetails": "%s",
  "api_key": "%s",
  "braveVPNEnabled": "%s",
  "channel": "%s",
  "contactInfo": "%s",
  "domain": "%s",
  "fpBlockSetting": "%s",
  "languageFarblingEnabled": "%s",
  "languages": "%s",
  "shieldsEnabled": "%s",
  "url": "%s",
  "version": "%s"
})",
          test_dict_string.c_str(), report.ad_block_list_names->c_str(),
          report.ad_block_setting->c_str(), report.details->c_str(),
          brave_stats::GetAPIKey().c_str(),
          report.brave_vpn_connected ? "true" : "false",
          report.channel->c_str(), report.contact->c_str(),
          url::Origin::Create(report.report_url.value()).Serialize().c_str(),
          report.fp_block_setting->c_str(),
          report.language_farbling ? "true" : "false",
          report.languages->c_str(), report.shields_enabled ? "true" : "false",
          report.report_url.value().spec().c_str(),
          report.brave_version->c_str()));
}

}  // namespace webcompat_reporter
