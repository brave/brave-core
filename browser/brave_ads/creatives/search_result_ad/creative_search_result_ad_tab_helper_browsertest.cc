/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/creatives/search_result_ad/creative_search_result_ad_tab_helper.h"

#include <memory>
#include <optional>

#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_browser_tests
// --filter=BraveAdsCreativeSearchResultAdTabHelperTest*

namespace brave_ads {

namespace {

constexpr char kAllowedDomain[] = "search.brave.com";
constexpr char kNotAllowedDomain[] = "brave.com";
constexpr char kClickRedirectPath[] = "/a/redirect";
constexpr char kTargetDomain[] = "example.com";
constexpr char kTargetPath[] = "/simple.html";
constexpr char kSearchResultUrlPath[] =
    "/brave_ads/creative_search_result_ad.html";
constexpr char kSearchResultClickedUrlPath[] =
    "/a/"
    "redirect?click_url=https%3A%2F%2Fexample.com%2Fsimple.html&placement_id="
    "824657d0-eaed-4b80-8a42-a18c12f2977d";

// Placement IDs are defined in the `search_result_ad_sample.html` file.
constexpr auto kCreativeAdPlacementIdToIndex =
    base::MakeFixedFlatMap<std::string_view, size_t>(
        {{"824657d0-eaed-4b80-8a42-a18c12f2977d", 1},
         {"fE%22%27%2B%2A%26-._~", 2}});

CreativeSearchResultAdTabHelper* GetCreativeSearchResultAdTabHelper(
    Browser* browser) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  return CreativeSearchResultAdTabHelper::FromWebContents(web_contents);
}

class ScopedTestingAdsServiceSetter {
 public:
  explicit ScopedTestingAdsServiceSetter(AdsService* const ads_service) {
    CreativeSearchResultAdTabHelper::SetAdsServiceForTesting(ads_service);
  }

  ~ScopedTestingAdsServiceSetter() {
    CreativeSearchResultAdTabHelper::SetAdsServiceForTesting(nullptr);
  }

  ScopedTestingAdsServiceSetter(const ScopedTestingAdsServiceSetter&) = delete;
  ScopedTestingAdsServiceSetter& operator=(
      const ScopedTestingAdsServiceSetter&) = delete;
};

}  // namespace

class BraveAdsCreativeSearchResultAdTabHelperTest
    : public CertVerifierBrowserTest {
 public:
  BraveAdsCreativeSearchResultAdTabHelperTest() {
    scoped_feature_list_.InitAndEnableFeature(
        kShouldSupportSearchResultAdsFeature);
  }

  void SetUpOnMainThread() override {
    CertVerifierBrowserTest::SetUpOnMainThread();
    mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.RegisterRequestHandler(base::BindRepeating(
        &BraveAdsCreativeSearchResultAdTabHelperTest::HandleRequest,
        base::Unretained(this)));

    const base::FilePath test_data_file_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_file_path);
    ASSERT_TRUE(https_server_.Start());
  }

  GURL GetURL(std::string_view domain, const std::string& path) const {
    base::StringPairs replacements;
    replacements.emplace_back(std::make_pair(
        "REPLACE_WITH_HTTP_PORT",
        base::NumberToString(https_server_.host_port_pair().port())));

    const std::string replaced_path =
        net::test_server::GetFilePathWithReplacements(path, replacements);
    return https_server_.GetURL(domain, replaced_path);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    const GURL url = request.GetURL();
    const std::string_view path = url.path_piece();

    if (!path.starts_with(kClickRedirectPath)) {
      return nullptr;
    }

    const GURL target_url = https_server_.GetURL(kTargetDomain, kTargetPath);
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
    http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
    http_response->AddCustomHeader("Location", target_url.spec());
    return http_response;
  }

  net::EmbeddedTestServer& https_server() { return https_server_; }

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  AdsServiceMock& ads_service() { return ads_service_mock_; }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  net::EmbeddedTestServer https_server_{
      net::test_server::EmbeddedTestServer::TYPE_HTTPS};
  AdsServiceMock ads_service_mock_{nullptr};
};

IN_PROC_BROWSER_TEST_F(BraveAdsCreativeSearchResultAdTabHelperTest,
                       UserHasNotJoinedBraveRewards) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  ScopedTestingAdsServiceSetter scoped_setter(&ads_service());
  EXPECT_CALL(ads_service(), TriggerSearchResultAdEvent).Times(0);

  const GURL url = GetURL(kAllowedDomain, kSearchResultUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(BraveAdsCreativeSearchResultAdTabHelperTest,
                       NotAllowedDomain) {
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service());

  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  EXPECT_CALL(ads_service(), TriggerSearchResultAdEvent).Times(0);

  const GURL url = GetURL(kNotAllowedDomain, kSearchResultUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(BraveAdsCreativeSearchResultAdTabHelperTest,
                       BrokenSearchAdMetadata) {
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service());

  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  EXPECT_CALL(ads_service(), TriggerSearchResultAdEvent).Times(0);

  const GURL url =
      GetURL(kAllowedDomain, "/brave_ads/invalid_creative_search_result_ad");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(BraveAdsCreativeSearchResultAdTabHelperTest,
                       IncognitoBrowser) {
  const GURL url = GetURL(kAllowedDomain, kSearchResultUrlPath);
  Browser* incognito_browser = OpenURLOffTheRecord(browser()->profile(), url);
  EXPECT_FALSE(GetCreativeSearchResultAdTabHelper(incognito_browser));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(incognito_browser, url));
  content::WebContents* web_contents =
      incognito_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());

  content::CreateAndLoadWebContentsObserver observer;
  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_2').click();"));
  observer.Wait();
}

class SampleBraveAdsCreativeSearchResultAdTabHelperTest
    : public BraveAdsCreativeSearchResultAdTabHelperTest {
 public:
  GURL GetSearchResultUrl() const {
    return GetURL(kAllowedDomain, kSearchResultUrlPath);
  }

  std::optional<size_t> GetIndexByPlacementId(
      const std::string& placement_id) const {
    const auto iter = kCreativeAdPlacementIdToIndex.find(placement_id);
    return iter != kCreativeAdPlacementIdToIndex.cend()
               ? std::optional<size_t>(iter->second)
               : std::nullopt;
  }

  mojom::CreativeSearchResultAdInfoPtr GenerateCreativeSearchResultAd(
      const std::string& placement_id) {
    auto mojom_creative_ad = mojom::CreativeSearchResultAdInfo::New();
    mojom_creative_ad->placement_id = placement_id;

    const auto iter =
        kCreativeAdPlacementIdToIndex.find(mojom_creative_ad->placement_id);
    if (iter == kCreativeAdPlacementIdToIndex.cend()) {
      return {};
    }
    const size_t ad_index = iter->second;

    const std::string index =
        base::StrCat({"-", base::NumberToString(ad_index)});
    mojom_creative_ad->creative_instance_id =
        base::StrCat({"data-creative-instance-id", index});
    mojom_creative_ad->creative_set_id =
        base::StrCat({"data-creative-set-id", index});
    mojom_creative_ad->campaign_id = base::StrCat({"data-campaign-id", index});
    mojom_creative_ad->advertiser_id =
        base::StrCat({"data-advertiser-id", index});
    mojom_creative_ad->target_url =
        GURL(base::StrCat({"https://foo.com/page", index}));
    mojom_creative_ad->headline_text =
        base::StrCat({"data-headline-text", index});
    mojom_creative_ad->description = base::StrCat({"data-description", index});
    mojom_creative_ad->value = 0.5 + ad_index;

    auto mojom_conversion = mojom::CreativeSetConversionInfo::New();
    mojom_conversion->url_pattern =
        base::StrCat({"data-conversion-url-pattern-value", index});
    if (ad_index == 1) {
      mojom_conversion->verifiable_advertiser_public_key_base64 =
          base::StrCat({"data-conversion-advertiser-public-key-value", index});
    }
    mojom_conversion->observation_window = base::Days(ad_index);
    mojom_creative_ad->creative_set_conversion = std::move(mojom_conversion);

    return mojom_creative_ad;
  }

  content::WebContents* LoadAndCheckSampleSearchResultAdWebPage(
      const GURL& url) {
    auto run_loop1 = std::make_unique<base::RunLoop>();
    auto run_loop2 = std::make_unique<base::RunLoop>();
    EXPECT_CALL(
        ads_service(),
        TriggerSearchResultAdEvent(
            ::testing::_, mojom::SearchResultAdEventType::kViewedImpression,
            ::testing::_))
        .Times(2)
        .WillRepeatedly(
            [this, &run_loop1, &run_loop2](
                mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                const mojom::SearchResultAdEventType mojom_ad_event_type,
                TriggerAdEventCallback callback) {
              ASSERT_TRUE(mojom_creative_ad);

              EXPECT_EQ(mojom_creative_ad,
                        GenerateCreativeSearchResultAd(
                            mojom_creative_ad->placement_id));

              const auto ad_index =
                  GetIndexByPlacementId(mojom_creative_ad->placement_id);

              if (ad_index == 1) {
                run_loop1->Quit();
              } else if (ad_index == 2) {
                run_loop2->Quit();
              }
            });

    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_EQ(url, web_contents->GetVisibleURL());

    run_loop1->Run();
    run_loop2->Run();

    return web_contents;
  }
};

IN_PROC_BROWSER_TEST_F(SampleBraveAdsCreativeSearchResultAdTabHelperTest,
                       SearchResultAdOpenedInSameTab) {
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service());

  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  content::WebContents* web_contents =
      LoadAndCheckSampleSearchResultAdWebPage(GetSearchResultUrl());

  EXPECT_CALL(ads_service(), MaybeGetSearchResultAd)
      .WillOnce([this](const std::string& placement_id,
                       MaybeGetSearchResultAdCallback callback) {
        std::move(callback).Run(GenerateCreativeSearchResultAd(placement_id));
      });

  base::RunLoop run_loop;
  EXPECT_CALL(ads_service(), TriggerSearchResultAdEvent)
      .WillOnce([this, &run_loop](
                    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                    const mojom::SearchResultAdEventType mojom_ad_event_type,
                    TriggerAdEventCallback callback) {
        EXPECT_EQ(mojom_ad_event_type,
                  mojom::SearchResultAdEventType::kClicked);

        // We clicked on the first ad in `search_result_ad_sample.html`.
        EXPECT_EQ(1u, GetIndexByPlacementId(mojom_creative_ad->placement_id));

        EXPECT_EQ(mojom_creative_ad, GenerateCreativeSearchResultAd(
                                         mojom_creative_ad->placement_id));
        run_loop.Quit();
      });

  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_1').click();"));
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(SampleBraveAdsCreativeSearchResultAdTabHelperTest,
                       SearchResultAdOpenedInNewTab) {
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service());

  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  content::WebContents* web_contents =
      LoadAndCheckSampleSearchResultAdWebPage(GetSearchResultUrl());

  base::RunLoop run_loop;
  EXPECT_CALL(ads_service(), TriggerSearchResultAdEvent)
      .WillOnce([this, &run_loop](
                    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                    const mojom::SearchResultAdEventType mojom_ad_event_type,
                    TriggerAdEventCallback callback) {
        EXPECT_EQ(mojom_ad_event_type,
                  mojom::SearchResultAdEventType::kClicked);

        // We clicked on the second ad in `search_result_ad_sample.html`.
        EXPECT_EQ(2u, GetIndexByPlacementId(mojom_creative_ad->placement_id));

        // VerifyCreativeAdMetadataExpectations(
        //     mojom_creative_ad, mojom_creative_ad->placement_id, ad_index);
        EXPECT_EQ(mojom_creative_ad, GenerateCreativeSearchResultAd(
                                         mojom_creative_ad->placement_id));
        run_loop.Quit();
      });

  EXPECT_CALL(ads_service(), MaybeGetSearchResultAd)
      .WillOnce([this](const std::string& placement_id,
                       MaybeGetSearchResultAdCallback callback) {
        std::move(callback).Run(GenerateCreativeSearchResultAd(placement_id));
      });

  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_2').click();"));
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(SampleBraveAdsCreativeSearchResultAdTabHelperTest,
                       SearchResultAdOpenedInNewTabByRightClick) {
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service());

  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  LoadAndCheckSampleSearchResultAdWebPage(GetSearchResultUrl());

  EXPECT_CALL(ads_service(), MaybeGetSearchResultAd)
      .WillOnce([this](const std::string& placement_id,
                       MaybeGetSearchResultAdCallback callback) {
        std::move(callback).Run(GenerateCreativeSearchResultAd(placement_id));
      });

  base::RunLoop run_loop;
  EXPECT_CALL(ads_service(), TriggerSearchResultAdEvent)
      .WillOnce([this, &run_loop](
                    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                    const mojom::SearchResultAdEventType mojom_ad_event_type,
                    TriggerAdEventCallback callback) {
        EXPECT_EQ(mojom_ad_event_type,
                  mojom::SearchResultAdEventType::kClicked);

        // We clicked on the first ad in `search_result_ad_sample.html`.
        EXPECT_EQ(1u, GetIndexByPlacementId(mojom_creative_ad->placement_id));

        // VerifyCreativeAdMetadataExpectations(
        //     mojom_creative_ad, mojom_creative_ad->placement_id, ad_index);
        EXPECT_EQ(mojom_creative_ad, GenerateCreativeSearchResultAd(
                                         mojom_creative_ad->placement_id));
        run_loop.Quit();
      });

  const GURL url =
      https_server().GetURL(kAllowedDomain, kSearchResultClickedUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(SampleBraveAdsCreativeSearchResultAdTabHelperTest,
                       SearchResultAdOpenedInNewWindow) {
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service());

  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  LoadAndCheckSampleSearchResultAdWebPage(GetSearchResultUrl());

  EXPECT_CALL(ads_service(), MaybeGetSearchResultAd)
      .WillOnce([this](const std::string& placement_id,
                       MaybeGetSearchResultAdCallback callback) {
        std::move(callback).Run(GenerateCreativeSearchResultAd(placement_id));
      });

  base::RunLoop run_loop;
  EXPECT_CALL(ads_service(), TriggerSearchResultAdEvent)
      .WillOnce([this, &run_loop](
                    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                    const mojom::SearchResultAdEventType mojom_ad_event_type,
                    TriggerAdEventCallback callback) {
        EXPECT_EQ(mojom_ad_event_type,
                  mojom::SearchResultAdEventType::kClicked);

        // We clicked on the first ad in `search_result_ad_sample.html`.
        EXPECT_EQ(1u, GetIndexByPlacementId(mojom_creative_ad->placement_id));

        // VerifyCreativeAdMetadataExpectations(
        //     mojom_creative_ad, mojom_creative_ad->placement_id, ad_index);
        EXPECT_EQ(mojom_creative_ad, GenerateCreativeSearchResultAd(
                                         mojom_creative_ad->placement_id));
        run_loop.Quit();
      });

  const GURL url =
      https_server().GetURL(kAllowedDomain, kSearchResultClickedUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  run_loop.Run();
}

}  // namespace brave_ads
