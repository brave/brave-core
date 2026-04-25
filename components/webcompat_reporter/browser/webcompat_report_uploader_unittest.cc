/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
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

  WebcompatReportUploader* GetWebcompatUploader() {
    return webcompat_report_uploader_.get();
  }

  base::DictValue TestReportGeneration(
      webcompat_reporter::mojom::ReportInfoPtr report) {
    std::optional<std::string> request_payload;
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&request_payload](const network::ResourceRequest& request) {
          ASSERT_TRUE(request_payload = network::GetUploadData(request));
        }));

    GetWebcompatUploader()->SubmitReport(std::move(report));

    EXPECT_TRUE(
        base::test::RunUntil([&]() { return request_payload.has_value(); }));

    return base::test::ParseJsonDict(*request_payload);
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  std::unique_ptr<WebcompatReportUploader> webcompat_report_uploader_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(WebcompatReportUploaderUnitTest, GenerateReport) {
  auto report_empty = webcompat_reporter::mojom::ReportInfo::New();
  EXPECT_EQ(TestReportGeneration(std::move(report_empty)),
            base::DictValue().Set("api_key", brave_stats::GetAPIKey()));

  std::vector<webcompat_reporter::mojom::ComponentInfoPtr> components;
  components.push_back(
      webcompat_reporter::mojom::ComponentInfo::New("name", "id", "version"));

  std::vector<std::string> webcompat_errors{"Could not create screenshot"};
  auto report = webcompat_reporter::mojom::ReportInfo::New(
      "dev", "1.231.45", "https://abc.url/p1/p2", "true",
      /*adblock_only_mode_enabled=*/"true", "ad_block_setting",
      "fp_block_setting", "ad_block_list_names", "languages", "true", "true",
      "category", "details", "contact", "block", "true", std::move(components),
      std::nullopt, webcompat_errors);

  base::ListValue errors_list;
  for (const auto& error : webcompat_errors) {
    errors_list.Append(error);
  }

  auto report_copy = report->Clone();

  EXPECT_EQ(
      TestReportGeneration(std::move(report)),
      base::DictValue()
          .Set("adBlockComponentsInfo",
               base::ListValue().Append(base::DictValue()
                                            .Set("id", "id")
                                            .Set("name", "name")
                                            .Set("version", "version")))
          .Set("adBlockLists", *report_copy->ad_block_list_names)
          .Set("adBlockSetting", *report_copy->ad_block_setting)
          .Set("additionalDetails", *report_copy->details)
          .Set("api_key", brave_stats::GetAPIKey())
          .Set("block_scripts", *report_copy->block_scripts == "true")
          .Set("braveVPNEnabled", report_copy->brave_vpn_connected.has_value())
          .Set("category", *report_copy->category)
          .Set("channel", *report_copy->channel)
          .Set("contactInfo", *report_copy->contact)
          .Set("cookie_policy", *report_copy->cookie_policy)
          .Set("domain",
               url::Origin::Create(GURL(report_copy->report_url.value()))
                   .Serialize())
          .Set("fpBlockSetting", *report_copy->fp_block_setting)
          .Set("languageFarblingEnabled",
               report_copy->language_farbling.has_value())
          .Set("languages", *report_copy->languages)
          .Set("shieldsEnabled", report_copy->shields_enabled.has_value())
          .Set("adblockOnlyModeEnabled",
               report_copy->adblock_only_mode_enabled.has_value())
          .Set("url", GURL(report_copy->report_url.value()).spec())
          .Set("version", *report_copy->brave_version)
          .Set("webcompatReportErrors", std::move(errors_list)));
}

}  // namespace webcompat_reporter
