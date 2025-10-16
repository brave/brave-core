/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/version_info/version_info.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "brave/components/webcompat_reporter/common/pref_names.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom-forward.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace {

constexpr char kChannelMockedValue[] = "MockedChannelValue";
constexpr char kCookiePolicyMockedValue[] = "MockedCookiePolicyValue";
constexpr char kGetScriptBlockingFlagMockedValue[] =
    "MockedScriptBlockingFlagValue";

class MockWebcompatReportUploader
    : public webcompat_reporter::WebcompatReportUploader {
 public:
  MockWebcompatReportUploader() : WebcompatReportUploader(nullptr) {}

  ~MockWebcompatReportUploader() override = default;
  MOCK_METHOD(void,
              SubmitReport,
              (webcompat_reporter::mojom::ReportInfoPtr report),
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
  MOCK_METHOD(std::vector<ComponentInfo>, GetComponentInfos, (), (const));
  MOCK_METHOD(std::optional<std::string>,
              GetCookiePolicy,
              (const std::optional<std::string>& current_url),
              (const));
  MOCK_METHOD(std::optional<std::string>,
              GetScriptBlockingFlag,
              (const std::optional<std::string>& current_url),
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
    prefs::RegisterProfilePrefs(pref_service_.registry());
    auto delegate = std::make_unique<MockWebCompatServiceDelegate>();
    delegate_ = delegate.get();
    webcompat_reporter_service_ = std::make_unique<WebcompatReporterService>(
        pref_service(), std::move(delegate),
        std::make_unique<MockWebcompatReportUploader>());
  }

  MockWebcompatReportUploader* GetMockWebcompatReportUploader() {
    return static_cast<MockWebcompatReportUploader*>(
        webcompat_reporter_service_->report_uploader_.get());
  }

  PrefService* pref_service() { return &pref_service_; }

  void TestSubmitWebcompatReport(const std::string contact,
                                 const bool is_incognito) {
    if (is_incognito) {
      webcompat_reporter_service_->SetPrefServiceTest(nullptr);
    }
    base::flat_map<std::string, std::string> key_val_data{{"key1", "val1"},
                                                          {"key2", "val2"}};
    std::vector<webcompat_reporter::mojom::ComponentInfoPtr> components;
    components.push_back(
        webcompat_reporter::mojom::ComponentInfo::New("name", "id", "version"));
    std::vector<uint8_t> screenshot{1, 2, 3, 4};
    std::vector<std::string> webcompat_errors{"Could not create screenshot"};
    auto report_info = webcompat_reporter::mojom::ReportInfo::New(
        "channel", "brave_version", "https://abc.url/p1/p2", "true",
        "ad_block_setting", "fp_block_setting", "ad_block_list_names",
        "languages", "true", "true", "category", "details", contact, "block",
        "true", std::move(components), screenshot, webcompat_errors);
    EXPECT_CALL(*GetMockWebcompatReportUploader(), SubmitReport(_))
        .Times(1)
        .WillOnce([&](webcompat_reporter::mojom::ReportInfoPtr report) {
          EXPECT_EQ(report->channel, "channel");
          EXPECT_EQ(report->brave_version, "brave_version");
          EXPECT_EQ(report->report_url, "https://abc.url/p1/p2");
          EXPECT_EQ(report->shields_enabled, "true");
          EXPECT_EQ(report->ad_block_setting, "ad_block_setting");
          EXPECT_EQ(report->fp_block_setting, "fp_block_setting");
          EXPECT_EQ(report->ad_block_list_names, "ad_block_list_names");
          EXPECT_EQ(report->languages, "languages");
          EXPECT_EQ(report->language_farbling, "true");
          EXPECT_EQ(report->brave_vpn_connected, "true");
          EXPECT_EQ(report->cookie_policy, "block");
          EXPECT_EQ(report->block_scripts, "true");

          EXPECT_EQ(report->category, "category");
          EXPECT_EQ(report->details, "details");
          EXPECT_EQ(report->contact, contact);

          EXPECT_TRUE(report->ad_block_components_version);
          EXPECT_FALSE(report->ad_block_components_version->empty());

          const auto& component = report->ad_block_components_version->front();
          EXPECT_EQ(component->id, "id");
          EXPECT_EQ(component->name, "name");
          EXPECT_EQ(component->version, "version");

          EXPECT_TRUE(report->screenshot_png);
          EXPECT_EQ(report->screenshot_png.value(), screenshot);
          EXPECT_EQ(report->webcompat_reporter_errors.value(),
                    webcompat_errors);
        });
    EXPECT_CALL(*delegate_, GetChannelName).Times(0);
    EXPECT_CALL(*delegate_, GetAdblockFilterListNames).Times(0);
    EXPECT_CALL(*delegate_, GetComponentInfos).Times(0);
    EXPECT_CALL(*delegate_, GetCookiePolicy).Times(0);
    EXPECT_CALL(*delegate_, GetScriptBlockingFlag).Times(0);

    webcompat_reporter_service_->SubmitWebcompatReport(std::move(report_info));
  }

  void TestRetrievingWebcompatCategories(
      const std::vector<mojom::WebcompatCategory>& required_categories,
      const std::vector<mojom::WebcompatCategory>& excluded_categories,
      const std::vector<ComponentInfo>& component_infos) {
    EXPECT_CALL(*delegate_, GetComponentInfos)
        .Times(1)
        .WillOnce(testing::Return(component_infos));

    base::test::TestFuture<std::vector<mojom::WebcompatCategoryItemPtr>> future;
    webcompat_reporter_service_->GetWebcompatCategories(future.GetCallback());

    auto categories = future.Take();
    EXPECT_EQ(categories.size(), required_categories.size());
    EXPECT_TRUE(std::ranges::all_of(required_categories, [&](auto item) {
      return std::ranges::find_if(
                 categories,
                 [&item](const mojom::WebcompatCategoryItemPtr& category) {
                   return category->category == item;
                 }) != categories.end();
    }));
    EXPECT_TRUE(std::ranges::none_of(excluded_categories, [&](auto item) {
      return std::ranges::find_if(
                 categories,
                 [&item](const mojom::WebcompatCategoryItemPtr& category) {
                   return category->category == item;
                 }) != categories.end();
    }));
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
  raw_ptr<MockWebCompatServiceDelegate, DanglingUntriaged> delegate_;
  std::unique_ptr<WebcompatReporterService> webcompat_reporter_service_;
};

TEST_F(WebcompatReporterServiceUnitTest, SubmitReport) {
  pref_service()->SetBoolean(prefs::kContactInfoSaveFlagPrefs, true);
  const std::string contact_value = "contact";
  TestSubmitWebcompatReport(contact_value, false);
  EXPECT_EQ(contact_value, pref_service()->GetString(prefs::kContactInfoPrefs));
}

TEST_F(WebcompatReporterServiceUnitTest, SubmitReportDoNotSaveContactInfo) {
  pref_service()->SetBoolean(prefs::kContactInfoSaveFlagPrefs, false);
  const std::string contact_value = "contact";
  TestSubmitWebcompatReport(contact_value, false);
  EXPECT_TRUE(pref_service()->GetString(prefs::kContactInfoPrefs).empty());
}

TEST_F(WebcompatReporterServiceUnitTest, SubmitReportIncognito) {
  pref_service()->SetBoolean(prefs::kContactInfoSaveFlagPrefs, true);
  const std::string contact_value = "contact";
  TestSubmitWebcompatReport(contact_value, true);
  EXPECT_EQ("", pref_service()->GetString(prefs::kContactInfoPrefs));
}

TEST_F(WebcompatReporterServiceUnitTest, SubmitReportWithNoPropsOverride) {
  std::vector<std::string> get_adblock_list_names_ret_value{"val1", "val2",
                                                            "val3"};
  auto report_info = webcompat_reporter::mojom::ReportInfo::New();
  std::vector<ComponentInfo> component_infos{
      {"adcocjohghhfpidemphmcmlmhnfgikei", "name", "1.234"}};
  EXPECT_CALL(*GetMockWebcompatReportUploader(), SubmitReport)
      .Times(1)
      .WillOnce([&](webcompat_reporter::mojom::ReportInfoPtr report) {
        // check values, which must be filled by the service
        EXPECT_TRUE(report->channel.has_value());
        EXPECT_EQ(report->channel, kChannelMockedValue);
        EXPECT_TRUE(report->brave_version.has_value());
        EXPECT_EQ(report->brave_version,
                  version_info::GetBraveVersionWithoutChromiumMajorVersion());
        EXPECT_TRUE(report->ad_block_components_version);
        EXPECT_EQ(report->ad_block_components_version->size(), 1u);
        EXPECT_EQ(report->ad_block_components_version->front()->id,
                  "adcocjohghhfpidemphmcmlmhnfgikei");
        EXPECT_TRUE(report->ad_block_list_names.has_value());
        EXPECT_EQ(base::JoinString(get_adblock_list_names_ret_value, ","),
                  report->ad_block_list_names.value());

        // check values, which we pass to the service
        EXPECT_FALSE(report->report_url);
        EXPECT_FALSE(report->shields_enabled);
        EXPECT_FALSE(report->category);
        EXPECT_FALSE(report->details);
        EXPECT_FALSE(report->contact);
        EXPECT_EQ(report->cookie_policy.value(), kCookiePolicyMockedValue);
        EXPECT_EQ(report->block_scripts.value(),
                  kGetScriptBlockingFlagMockedValue);
        EXPECT_FALSE(report->fp_block_setting);
        EXPECT_FALSE(report->ad_block_setting);
        EXPECT_FALSE(report->language_farbling);
        EXPECT_FALSE(report->brave_vpn_connected);
        EXPECT_FALSE(report->languages);
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
  EXPECT_CALL(*delegate_, GetCookiePolicy)
      .Times(1)
      .WillOnce(testing::Return(kCookiePolicyMockedValue));
  EXPECT_CALL(*delegate_, GetScriptBlockingFlag)
      .Times(1)
      .WillOnce(testing::Return(kGetScriptBlockingFlagMockedValue));

  webcompat_reporter_service_->SubmitWebcompatReport(std::move(report_info));
}

TEST_F(WebcompatReporterServiceUnitTest, RetrievingWebcompatCategories) {
  {
    std::vector<ComponentInfo> component_infos{
        {"adcocjohghhfpidemphmcmlmhnfgikei", "name", "1.234"}};
    TestRetrievingWebcompatCategories(
        {mojom::WebcompatCategory::kAds, mojom::WebcompatCategory::kBlank,
         mojom::WebcompatCategory::kScroll, mojom::WebcompatCategory::kForm,
         mojom::WebcompatCategory::kAntiAdblock,
         mojom::WebcompatCategory::kTracking,
         mojom::WebcompatCategory::kBrowserNotSupported,
         mojom::WebcompatCategory::kOther},
        {mojom::WebcompatCategory::kCookieNotice,
         mojom::WebcompatCategory::kNewsletter,
         mojom::WebcompatCategory::kSocial, mojom::WebcompatCategory::kChat},
        component_infos);
  }
  {
    std::vector<ComponentInfo> component_infos{
        {"adcocjohghhfpidemphmcmlmhnfgikei", "name", "1.234"},
        // In this component is present, WebcompatCategory::kCookieNotice must
        // be shown
        {"cdbbhgbmjhfnhnmgeddbliobbofkgdhe", "name1", "2.234"},
        // In this component is present, WebcompatCategory::kNewsletter must be
        // shown
        {"kdddfellohomdnfkdhombbddhojklibj", "name2", "3.234"},
        // In this component is present, WebcompatCategory::kSocial must be
        // shown
        {"nbkknaieglghmocpollinelcggiehfco", "name3", "4.234"},
        // In this component is present, WebcompatCategory::kChat must be
        // shown
        {"cjoooeeofnfjohnalnghhmdlalopplja", "name4", "5.234"}};
    TestRetrievingWebcompatCategories(
        {mojom::WebcompatCategory::kAds, mojom::WebcompatCategory::kBlank,
         mojom::WebcompatCategory::kScroll, mojom::WebcompatCategory::kForm,
         mojom::WebcompatCategory::kCookieNotice,
         mojom::WebcompatCategory::kAntiAdblock,
         mojom::WebcompatCategory::kTracking,
         mojom::WebcompatCategory::kNewsletter,
         mojom::WebcompatCategory::kSocial, mojom::WebcompatCategory::kChat,
         mojom::WebcompatCategory::kBrowserNotSupported,
         mojom::WebcompatCategory::kOther},
        {}, component_infos);
  }
  {
    std::vector<ComponentInfo> component_infos{
        {"adcocjohghhfpidemphmcmlmhnfgikei", "name", "1.234"},
        // In this component is present, WebcompatCategory::kCookieNotice must
        // be shown
        {"cdbbhgbmjhfnhnmgeddbliobbofkgdhe", "name1", "2.234"}};
    TestRetrievingWebcompatCategories(
        {mojom::WebcompatCategory::kAds, mojom::WebcompatCategory::kBlank,
         mojom::WebcompatCategory::kScroll, mojom::WebcompatCategory::kForm,
         mojom::WebcompatCategory::kCookieNotice,
         mojom::WebcompatCategory::kAntiAdblock,
         mojom::WebcompatCategory::kTracking,
         mojom::WebcompatCategory::kBrowserNotSupported,
         mojom::WebcompatCategory::kOther},
        {mojom::WebcompatCategory::kNewsletter,
         mojom::WebcompatCategory::kSocial, mojom::WebcompatCategory::kChat},
        component_infos);
  }
}

}  // namespace webcompat_reporter
