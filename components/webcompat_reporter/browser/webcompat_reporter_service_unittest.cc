/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/version_info/version_info.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom-forward.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

using testing::_;

namespace {

constexpr char kChannelMockedValue[] = "MockedChannelValue";

class MockWebcompatReportUploader
    : public webcompat_reporter::WebcompatReportUploader {
 public:
  MockWebcompatReportUploader() : WebcompatReportUploader(nullptr) {}

  ~MockWebcompatReportUploader() override = default;
  MOCK_METHOD(void,
              SubmitReport,
              (const webcompat_reporter::Report& report),
              (override));
};

using WebCompatServiceDelegate =
    webcompat_reporter::WebcompatReporterService::Delegate;
class MockWebCompatServiceDelegate : public WebCompatServiceDelegate {
 public:
  MockWebCompatServiceDelegate() = default;
  ~MockWebCompatServiceDelegate() override = default;

  MOCK_METHOD(std::optional<std::vector<std::string>>,
              GetAdblockFilterListNames,
              (),
              (const));
  MOCK_METHOD(std::optional<std::string>, GetChannelName, (), (const));
  MOCK_METHOD(std::optional<std::vector<ComponentInfo>>,
              GetComponentInfos,
              (),
              (const));
};
}  // namespace

namespace webcompat_reporter {

class WebcompatReporterServiceUnitTest : public testing::Test {
 public:
  using ComponentInfo = WebcompatReporterService::Delegate::ComponentInfo;
  WebcompatReporterServiceUnitTest() = default;
  ~WebcompatReporterServiceUnitTest() override = default;

  void SetUp() override {
    auto delegate = std::make_unique<MockWebCompatServiceDelegate>();
    delegate_ = delegate.get();
    webcompat_reporter_service_ = std::make_unique<WebcompatReporterService>(
        std::move(delegate), nullptr);

    webcompat_reporter_service_->SetReportUploaderForTest(
        std::unique_ptr<WebcompatReportUploader>(
            new MockWebcompatReportUploader()));
  }

  MockWebcompatReportUploader* GetMockWebcompatReportUploader() {
    return static_cast<MockWebcompatReportUploader*>(
        webcompat_reporter_service_->report_uploader_.get());
  }

 protected:
  raw_ptr<MockWebCompatServiceDelegate> delegate_;
  std::unique_ptr<WebcompatReporterService> webcompat_reporter_service_;
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_F(WebcompatReporterServiceUnitTest, SubmitReportWithReportPropsOverride) {
  Report report_source;
  report_source.channel = "channel";
  report_source.brave_version = "1.231.45";
  report_source.shields_enabled = "true";
  report_source.language_farbling = "true";
  report_source.brave_vpn_connected = "true";
  report_source.ad_block_setting = "ad_block_setting";
  report_source.fp_block_setting = "fp_block_setting";
  report_source.ad_block_list_names = "ad_block_list_names";
  report_source.languages = "languages";
  base::Value::Dict test_dict;
  test_dict.Set("key1", "val1");
  test_dict.Set("key1", "val1");
  report_source.ad_block_components = base::Value(test_dict.Clone());
  report_source.details = "details";
  report_source.contact = "contact";
  report_source.report_url = GURL("https://abc.url/p1/p2");

  EXPECT_CALL(*GetMockWebcompatReportUploader(), SubmitReport)
      .Times(1)
      .WillOnce([&](const Report& report) {
        // all values passed to the service
        EXPECT_EQ(report_source.channel, report.channel);
        EXPECT_EQ(report_source.brave_version, report.brave_version);
        EXPECT_EQ(report_source.shields_enabled, report.shields_enabled);
        EXPECT_EQ(report_source.language_farbling, report.language_farbling);
        EXPECT_EQ(report_source.brave_vpn_connected,
                  report.brave_vpn_connected);
        EXPECT_EQ(report_source.ad_block_setting, report.ad_block_setting);
        EXPECT_EQ(report_source.fp_block_setting, report.fp_block_setting);
        EXPECT_EQ(report_source.ad_block_list_names,
                  report.ad_block_list_names);
        EXPECT_EQ(report_source.languages, report.languages);
        EXPECT_EQ(report_source.ad_block_components,
                  report.ad_block_components);
        EXPECT_EQ(report_source.details, report.details);
        EXPECT_EQ(report_source.contact, report.contact);
        EXPECT_EQ(report_source.report_url, report.report_url);
      });
  EXPECT_CALL(*delegate_, GetComponentInfos).Times(0);
  EXPECT_CALL(*delegate_, GetChannelName).Times(0);
  EXPECT_CALL(*delegate_, GetAdblockFilterListNames).Times(0);
  webcompat_reporter_service_->SubmitWebcompatReport(report_source);
}

TEST_F(WebcompatReporterServiceUnitTest, SubmitReportWithNoPropsOverride) {
  Report report_source;

  std::vector<ComponentInfo> component_infos{
      {"adcocjohghhfpidemphmcmlmhnfgikei", "name", "1.234"}};

  std::vector<std::string> get_adblock_list_names_ret_value{"val1", "val2",
                                                            "val3"};
  EXPECT_CALL(*GetMockWebcompatReportUploader(), SubmitReport)
      .Times(1)
      .WillOnce([&](const Report& report) {
        // check values, which must be filled by the service
        EXPECT_TRUE(report.channel.has_value());
        EXPECT_EQ(report.channel, kChannelMockedValue);
        EXPECT_TRUE(report.brave_version.has_value());
        EXPECT_EQ(report.brave_version,
                  version_info::GetBraveVersionWithoutChromiumMajorVersion());
        EXPECT_TRUE(report.ad_block_components->is_list());
        const auto& ad_block_components = report.ad_block_components->GetList();
        EXPECT_EQ(ad_block_components.size(), 1u);
        EXPECT_TRUE(ad_block_components.front().is_dict());
        EXPECT_EQ(*ad_block_components.front().GetDict().FindString("id"),
                  "adcocjohghhfpidemphmcmlmhnfgikei");

        // check values, which we pass to the service
        EXPECT_EQ(report_source.report_url, report.report_url);
        EXPECT_EQ(report_source.shields_enabled, report.shields_enabled);
        EXPECT_EQ(report_source.details, report.details);
        EXPECT_EQ(report_source.contact, report.contact);
        EXPECT_EQ(report_source.fp_block_setting, report.fp_block_setting);
        EXPECT_EQ(report_source.ad_block_setting, report.ad_block_setting);
        EXPECT_EQ(report_source.language_farbling, report.language_farbling);
        EXPECT_EQ(report_source.brave_vpn_connected,
                  report.brave_vpn_connected);

        EXPECT_TRUE(report.ad_block_list_names.has_value());
        EXPECT_EQ(base::JoinString(get_adblock_list_names_ret_value, ","),
                  report.ad_block_list_names.value());
        EXPECT_EQ(report_source.languages, report.languages);
      });
  EXPECT_CALL(*delegate_, GetComponentInfos)
      .Times(1)
      .WillOnce(testing::Return(component_infos));
  EXPECT_CALL(*delegate_, GetChannelName)
      .Times(1)
      .WillOnce(testing::Return(kChannelMockedValue));
  EXPECT_CALL(*delegate_, GetAdblockFilterListNames)
      .Times(1)
      .WillOnce(testing::Return(get_adblock_list_names_ret_value));

  webcompat_reporter_service_->SubmitWebcompatReport(report_source);
}

TEST_F(WebcompatReporterServiceUnitTest, SubmitReportMojo) {
  base::flat_map<std::string, std::string> key_val_data{{"key1", "val1"},
                                                        {"key2", "val2"}};
  std::vector<webcompat_reporter::mojom::ComponentInfoPtr> components;
  components.push_back(
      webcompat_reporter::mojom::ComponentInfo::New("name", "id", "version"));
  std::vector<uint8_t> screenshot{1, 2, 3, 4};
  auto report_info = webcompat_reporter::mojom::ReportInfo::New(
      "channel", "brave_version", "https://abc.url/p1/p2", "true",
      "ad_block_setting", "fp_block_setting", "ad_block_list_names",
      "languages", "true", "true", "details", "contact", std::move(components),
      screenshot);

  EXPECT_CALL(*GetMockWebcompatReportUploader(), SubmitReport(_))
      .Times(1)
      .WillOnce([&](const Report& report) {
        EXPECT_EQ(report.channel, "channel");
        EXPECT_EQ(report.brave_version, "brave_version");
        EXPECT_EQ(report.report_url, "https://abc.url/p1/p2");
        EXPECT_EQ(report.shields_enabled, "true");
        EXPECT_EQ(report.ad_block_setting, "ad_block_setting");
        EXPECT_EQ(report.fp_block_setting, "fp_block_setting");
        EXPECT_EQ(report.ad_block_list_names, "ad_block_list_names");
        EXPECT_EQ(report.languages, "languages");
        EXPECT_EQ(report.language_farbling, "true");
        EXPECT_EQ(report.brave_vpn_connected, "true");

        EXPECT_EQ(report.details, "details");
        EXPECT_EQ(report.contact, "contact");

        EXPECT_TRUE(report.ad_block_components);
        EXPECT_TRUE(report.ad_block_components->is_list());

        EXPECT_TRUE(report.ad_block_components);
        EXPECT_TRUE(report.ad_block_components->is_list());
        EXPECT_FALSE(report.ad_block_components->GetList().empty());
        const auto& component = report.ad_block_components->GetList().front();
        EXPECT_TRUE(component.is_dict());
        EXPECT_EQ(*component.GetDict().FindString("id"), "id");
        EXPECT_EQ(*component.GetDict().FindString("name"), "name");
        EXPECT_EQ(*component.GetDict().FindString("version"), "version");

        EXPECT_TRUE(report.screenshot_png);
        EXPECT_EQ(report.screenshot_png.value(), screenshot);
      });
  EXPECT_CALL(*delegate_, GetChannelName).Times(0);
  EXPECT_CALL(*delegate_, GetAdblockFilterListNames).Times(0);
  EXPECT_CALL(*delegate_, GetComponentInfos).Times(0);

  webcompat_reporter_service_->SubmitWebcompatReport(std::move(report_info));
}

TEST_F(WebcompatReporterServiceUnitTest, SubmitReportMojoWithNoPropsOverride) {
  std::vector<std::string> get_adblock_list_names_ret_value{"val1", "val2",
                                                            "val3"};
  auto report_info = webcompat_reporter::mojom::ReportInfo::New();
  std::vector<ComponentInfo> component_infos{
      {"adcocjohghhfpidemphmcmlmhnfgikei", "name", "1.234"}};
  EXPECT_CALL(*GetMockWebcompatReportUploader(), SubmitReport)
      .Times(1)
      .WillOnce([&](const Report& report) {
        // check values, which must be filled by the service
        EXPECT_TRUE(report.channel.has_value());
        EXPECT_EQ(report.channel, kChannelMockedValue);
        EXPECT_TRUE(report.brave_version.has_value());
        EXPECT_EQ(report.brave_version,
                  version_info::GetBraveVersionWithoutChromiumMajorVersion());
        EXPECT_TRUE(report.ad_block_components->is_list());
        const auto& ad_block_components = report.ad_block_components->GetList();
        EXPECT_EQ(ad_block_components.size(), 1u);
        EXPECT_TRUE(ad_block_components.front().is_dict());
        EXPECT_EQ(*ad_block_components.front().GetDict().FindString("id"),
                  "adcocjohghhfpidemphmcmlmhnfgikei");
        EXPECT_TRUE(report.ad_block_list_names.has_value());
        EXPECT_EQ(base::JoinString(get_adblock_list_names_ret_value, ","),
                  report.ad_block_list_names.value());

        // check values, which we pass to the service
        EXPECT_FALSE(report.report_url);
        EXPECT_FALSE(report.shields_enabled);
        EXPECT_FALSE(report.details);
        EXPECT_FALSE(report.contact);
        EXPECT_FALSE(report.fp_block_setting);
        EXPECT_FALSE(report.ad_block_setting);
        EXPECT_FALSE(report.language_farbling);
        EXPECT_FALSE(report.brave_vpn_connected);
        EXPECT_FALSE(report.languages);
      });
  EXPECT_CALL(*delegate_, GetComponentInfos)
      .Times(1)
      .WillOnce(testing::Return(component_infos));
  EXPECT_CALL(*delegate_, GetChannelName)
      .Times(1)
      .WillOnce(testing::Return(kChannelMockedValue));
  EXPECT_CALL(*delegate_, GetAdblockFilterListNames)
      .Times(1)
      .WillOnce(testing::Return(get_adblock_list_names_ret_value));

  webcompat_reporter_service_->SubmitWebcompatReport(std::move(report_info));
}

}  // namespace webcompat_reporter
