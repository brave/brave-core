/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/types/optional_util.h"
#include "base/values.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/creatives/search_result_ad/test/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/browser/network/oblivious_http_feature.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_waiter.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/user_engagement/conversions/conversions_feature.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/url_loader_interceptor.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr std::string_view kAllowedDomain = "search.brave.com";
constexpr std::string_view kClickRedirectPath = "/a/redirect";
constexpr std::string_view kTargetDomain = "example.com";
constexpr std::string_view kTargetPath = "/simple.html";
constexpr std::string_view kSearchResultWithConversionUrlPath =
    "/brave_ads/creative_search_result_ad_with_conversion.html";
constexpr std::string_view kConversionUrlPath = "/simple_link.html";
constexpr std::string_view kSinglePageApplicationConversionUrlPath =
    "/brave_ads/single_page_application_conversion.html";
constexpr std::string_view kPlacementId =
    "824657d0-eaed-4b80-8a42-a18c12f2977d";
constexpr std::string_view kAdLinkId = "ad_link_1";
constexpr std::string_view kCreativeInstanceId = "data-creative-instance-id-1";
constexpr std::string_view kSecondPlacementId =
    "3f3b5bd1-0bd8-4a3e-9d2c-5f3c0f8a3f0b";
constexpr std::string_view kSecondAdLinkId = "ad_link_2";
constexpr std::string_view kSecondCreativeInstanceId =
    "data-creative-instance-id-2";
constexpr std::string_view kConversionLinkId = "conversion_link";
constexpr std::string_view kNonConversionLinkId = "non_conversion_link";
constexpr std::string_view kSecondConversionLinkId = "second_conversion_link";
constexpr std::string_view kStagingAnonymousSearchConfirmationHost =
    "search.anonymous.ads.bravesoftware.com";
constexpr std::string_view kConfirmationPathPrefix = "/v4/confirmation/";

std::optional<std::string> MaybeGetConversionConfirmationCreativeInstanceId(
    const network::ResourceRequest& request) {
  if (request.method != net::HttpRequestHeaders::kPostMethod ||
      request.url.host() != kStagingAnonymousSearchConfirmationHost ||
      !request.url.path().starts_with(kConfirmationPathPrefix) ||
      !request.request_body || request.request_body->elements()->empty()) {
    return std::nullopt;
  }

  const network::DataElement& element =
      request.request_body->elements()->front();
  if (element.type() != network::DataElement::Tag::kBytes) {
    return std::nullopt;
  }

  const std::optional<base::DictValue> dict = base::JSONReader::ReadDict(
      element.As<network::DataElementBytes>().AsStringView(),
      base::JSON_PARSE_RFC);
  if (!dict) {
    return std::nullopt;
  }

  const std::string* const type = dict->FindString("type");
  if (!type || *type != "conversion") {
    return std::nullopt;
  }

  return base::OptionalFromPtr(dict->FindString("creativeInstanceId"));
}

}  // namespace

class BraveAdsCreativeSearchResultAdConversionTest
    : public CertVerifierBrowserTest {
 public:
  BraveAdsCreativeSearchResultAdConversionTest() {
    ohttp_scoped_feature_list_.InitAndDisableFeature(kAdsObliviousHttpFeature);
    conversions_scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kConversionsFeature, {{"campaign_conversion_cap", "0"}});
  }

  void SetUpOnMainThread() override {
    CertVerifierBrowserTest::SetUpOnMainThread();
    mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.RegisterRequestHandler(base::BindRepeating(
        &BraveAdsCreativeSearchResultAdConversionTest::HandleRequest,
        base::Unretained(this)));

    const base::FilePath test_data_file_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_file_path);
    ASSERT_TRUE(https_server_.Start());

    conversion_test_future_ =
        std::make_unique<base::test::TestFuture<std::string>>(
            base::test::TestFutureMode::kQueue);
    url_loader_interceptor_ = InterceptConversionConfirmations(
        conversion_test_future_->GetSequenceBoundRepeatingCallback());
  }

  void TearDownOnMainThread() override {
    url_loader_interceptor_.reset();
    conversion_test_future_.reset();

    CertVerifierBrowserTest::TearDownOnMainThread();
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    if (!request.GetURL().path().starts_with(kClickRedirectPath)) {
      return nullptr;
    }

    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
    http_response->AddCustomHeader(
        "Location", https_server_.GetURL(kTargetDomain, kTargetPath).spec());
    return http_response;
  }

  void LoadAndClickSearchResultAd(
      std::string_view placement_id,
      std::string_view ad_link_id,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());

    ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));

    AdsService* ads_service =
        AdsServiceFactory::GetForProfile(browser()->GetProfile());
    ASSERT_TRUE(ads_service);
    test::AdsServiceWaiter(*ads_service).WaitForOnDidInitializeAdsService();

    base::StringPairs replacements;
    replacements.emplace_back(
        "REPLACE_WITH_HTTP_PORT",
        base::NumberToString(https_server_.host_port_pair().port()));
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(),
        https_server_.GetURL(
            kAllowedDomain, net::test_server::GetFilePathWithReplacements(
                                std::string(kSearchResultWithConversionUrlPath),
                                replacements))));
    content::WebContents* web_contents =
        chrome_test_utils::GetActiveWebContents(this);

    browser()->GetProfile()->GetPrefs()->SetBoolean(
        prefs::kShouldShowSearchResultAdClickedInfoBar, true);

    ASSERT_TRUE(content::ExecJs(
        web_contents, content::JsReplace("document.getElementById($1).click();",
                                         ad_link_id)));
    ASSERT_TRUE(content::WaitForLoadStop(web_contents));
    ASSERT_TRUE(base::test::RunUntil([web_contents] {
      return test::HasSearchResultAdClickedInfoBar(web_contents);
    }));
  }

  void NavigateToSinglePageApplicationConversionPage() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(),
        https_server_.GetURL(kTargetDomain,
                             kSinglePageApplicationConversionUrlPath)));
  }

  void ClickLinkAndWaitForNavigation(
      std::string_view link_id,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());

    content::WebContents* web_contents =
        chrome_test_utils::GetActiveWebContents(this);
    content::TestNavigationObserver navigation_observer(web_contents);
    ASSERT_TRUE(content::ExecJs(
        web_contents,
        content::JsReplace("document.getElementById($1).click();", link_id)));
    navigation_observer.Wait();
    ASSERT_TRUE(navigation_observer.last_navigation_succeeded());
  }

  void WaitForConversion() {
    ASSERT_TRUE(conversion_test_future_->Wait())
        << "Timed out waiting for a conversion confirmation to "
        << kStagingAnonymousSearchConfirmationHost;
    converted_creative_instance_ids_.push_back(conversion_test_future_->Take());
  }

  const std::vector<std::string>& converted_creative_instance_ids() const {
    return converted_creative_instance_ids_;
  }

  net::EmbeddedTestServer& https_server() { return https_server_; }

 private:
  std::unique_ptr<content::URLLoaderInterceptor>
  InterceptConversionConfirmations(
      base::RepeatingCallback<void(std::string)> callback) {
    return std::make_unique<content::URLLoaderInterceptor>(base::BindRepeating(
        [](const base::RepeatingCallback<void(std::string)>& callback,
           content::URLLoaderInterceptor::RequestParams* params) {
          std::optional<std::string> creative_instance_id =
              MaybeGetConversionConfirmationCreativeInstanceId(
                  params->url_request);
          if (!creative_instance_id) {
            return false;
          }

          content::URLLoaderInterceptor::WriteResponse(
              "HTTP/1.1 201 Created\nContent-Type: application/json\n\n", "{}",
              params->client.get());
          callback.Run(std::move(*creative_instance_id));
          return true;
        },
        std::move(callback)));
  }

  base::test::ScopedFeatureList ohttp_scoped_feature_list_;
  base::test::ScopedFeatureList conversions_scoped_feature_list_;
  net::EmbeddedTestServer https_server_{
      net::test_server::EmbeddedTestServer::TYPE_HTTPS};
  std::unique_ptr<base::test::TestFuture<std::string>> conversion_test_future_;
  std::unique_ptr<content::URLLoaderInterceptor> url_loader_interceptor_;
  std::vector<std::string> converted_creative_instance_ids_;
};

IN_PROC_BROWSER_TEST_F(
    BraveAdsCreativeSearchResultAdConversionTest,
    ConvertsClickedSearchResultAdWhenNavigatingToConversionUrl) {
  // Arrange
  LoadAndClickSearchResultAd(kPlacementId, kAdLinkId);

  // Act & Assert
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server().GetURL(kTargetDomain, kConversionUrlPath)));
  WaitForConversion();
  EXPECT_THAT(converted_creative_instance_ids(),
              ::testing::ElementsAre(kCreativeInstanceId));
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsCreativeSearchResultAdConversionTest,
    ConvertsClickedSearchResultAdWhenSameDocumentNavigatingToConversionUrl) {
  // Arrange
  LoadAndClickSearchResultAd(kPlacementId, kAdLinkId);
  NavigateToSinglePageApplicationConversionPage();

  // Act & Assert
  ClickLinkAndWaitForNavigation(kConversionLinkId);
  WaitForConversion();
  EXPECT_THAT(converted_creative_instance_ids(),
              ::testing::ElementsAre(kCreativeInstanceId));
}

IN_PROC_BROWSER_TEST_F(
    BraveAdsCreativeSearchResultAdConversionTest,
    DoesNotConvertClickedSearchResultAdAgainWhenRevisitingConversionUrl) {
  // Arrange
  LoadAndClickSearchResultAd(kPlacementId, kAdLinkId);
  NavigateToSinglePageApplicationConversionPage();
  ClickLinkAndWaitForNavigation(kConversionLinkId);
  WaitForConversion();
  ASSERT_THAT(converted_creative_instance_ids(),
              ::testing::ElementsAre(kCreativeInstanceId));

  // Act & Assert
  ClickLinkAndWaitForNavigation(kNonConversionLinkId);
  ClickLinkAndWaitForNavigation(kConversionLinkId);

  LoadAndClickSearchResultAd(kSecondPlacementId, kSecondAdLinkId);
  NavigateToSinglePageApplicationConversionPage();
  ClickLinkAndWaitForNavigation(kSecondConversionLinkId);
  WaitForConversion();
  EXPECT_THAT(
      converted_creative_instance_ids(),
      ::testing::ElementsAre(kCreativeInstanceId, kSecondCreativeInstanceId));
}

}  // namespace brave_ads
