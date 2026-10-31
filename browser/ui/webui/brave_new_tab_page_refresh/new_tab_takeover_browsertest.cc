/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <string>

#include "base/callback_list.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service_waiter.h"
#include "brave/components/ntp_background_images/browser/switches.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/sessions/session_restore_test_helper.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "url/gurl.h"

namespace {

std::unique_ptr<KeyedService> CreateAdsService(
    content::BrowserContext* const /*context*/) {
  return std::make_unique<brave_ads::AdsServiceMock>();
}

void OnWillCreateBrowserContextServices(
    content::BrowserContext* const context) {
  brave_ads::AdsServiceFactory::GetInstance()->SetTestingFactory(
      context, base::BindRepeating(&CreateAdsService));
}

brave_ads::mojom::NewTabPageAdInfoPtr BuildNewTabPageAd() {
  brave_ads::mojom::NewTabPageAdInfoPtr ad =
      brave_ads::mojom::NewTabPageAdInfo::New();
  ad->placement_id = "326eb47b-467b-46ab-ac1b-5f5de780b344";
  ad->creative_instance_id = "aa0b561e-9eed-4aaa-8999-5627bc6b14fd";
  ad->creative_set_id = "34ab06be-c9ed-4104-9ce0-9e639f4ad272";
  ad->campaign_id = "65933e82-6b21-440b-9956-c0f675ca7435";
  ad->advertiser_id = "496b045a-195e-441f-b439-07bac083450f";
  ad->segment = "untargeted";
  ad->target_url = GURL("https://basicattentiontoken.org");
  ad->company_name = "Image NTT Creative";
  ad->alt = "Some content";
  return ad;
}

}  // namespace

class NewTabTakeoverBrowserTest : public InProcessBrowserTest {
 public:
  NewTabTakeoverBrowserTest() {
    // Set New Tab Takeover to be displayed on every new tab for the test.
    // This is needed because there is no NTP background images data available
    // for tests, so the view counter is not incremented when trying to display
    // NTP background.
    // TODO(https://github.com/brave/brave-browser/issues/51437): Add proper
    // values for kInitialCountToNewTabTakeoverWallpaper and
    // kCountToNewTabTakeoverWallpaper once NTP background images data is
    // available for tests.
    base::FieldTrialParams parameters;
    std::vector<base::test::FeatureRefAndParams> enabled_features;
    parameters[ntp_background_images::features::
                   kInitialCountToNewTabTakeoverWallpaper.name] = "1";
    parameters[ntp_background_images::features::kCountToNewTabTakeoverWallpaper
                   .name] = "1";
    enabled_features.emplace_back(
        ntp_background_images::features::kBraveNTPNewTabTakeoverWallpaper,
        parameters);
    feature_list_.InitWithFeaturesAndParameters(enabled_features, {});
  }
  ~NewTabTakeoverBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    const base::FilePath test_data_file_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);

    const base::FilePath component_file_path =
        test_data_file_path.AppendASCII("components")
            .AppendASCII("ntp_sponsored_images")
            .AppendASCII("new_tab_takeover")
            .AppendASCII(GetComponentDataDirectoryName());
    base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
        ntp_background_images::switches::kOverrideSponsoredImagesComponentPath,
        component_file_path);

    ntp_background_images::NTPBackgroundImagesService* const
        ntp_background_images_service =
            g_brave_browser_process->ntp_background_images_service();
    ASSERT_TRUE(ntp_background_images_service);

    ntp_background_images::NTPBackgroundImagesServiceWaiter waiter(
        *ntp_background_images_service);
    ntp_background_images_service->Init();
    waiter.WaitForOnSponsoredContentDidUpdate();
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();

    callback_list_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(
                base::BindRepeating(&OnWillCreateBrowserContextServices));
  }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  brave_ads::AdsServiceMock& GetAdsServiceMock() {
    brave_ads::AdsService* ads_service =
        brave_ads::AdsServiceFactory::GetForProfile(
            chrome_test_utils::GetProfile(this));
    CHECK(ads_service);
    return *static_cast<brave_ads::AdsServiceMock*>(ads_service);
  }

  void CloseBrowserAndRestoreSession() {
    Profile* const profile = GetProfile();
    const ScopedKeepAlive scoped_keep_alive(KeepAliveOrigin::SESSION_RESTORE,
                                            KeepAliveRestartOption::DISABLED);
    ScopedProfileKeepAlive test_profile_keep_alive(
        profile, ProfileKeepAliveOrigin::kSessionRestore);
    CloseBrowserSynchronously(browser());

    ui_test_utils::BrowserCreatedObserver browser_created_observer;
    SessionRestoreTestHelper session_restore_test_helper;
    chrome::OpenWindowWithRestoredTabs(profile);
    if (SessionRestore::IsRestoring(profile)) {
      session_restore_test_helper.Wait();
    }
    SetBrowser(browser_created_observer.Wait());
  }

  void WaitForLoadStop() {
    EXPECT_TRUE(content::WaitForLoadStop(GetActiveWebContents()));
  }

  void OpenNewTabAndWaitForLoad() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                             GURL(chrome::kChromeUINewTabURL)));
    WaitForLoadStop();
  }

  void VerifyNewTabPageLoadedExpectation() {
    EXPECT_TRUE(content::EvalJs(GetActiveWebContents(),
                                "!!document.querySelector(`html[data-testid="
                                "'brave-new-tab-page']`)")
                    .ExtractBool());
  }

  void WaitForNewTabTakeoverDisclosureLabel(
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    EXPECT_TRUE(content::EvalJs(GetActiveWebContents(), R"(
      new Promise((resolve) => {
        const isLabelPresent = () =>
            !!document.querySelector('.new-tab-takeover-disclosure');
        if (isLabelPresent()) {
          resolve(true);
          return;
        }
        const observer = new MutationObserver(() => {
          if (isLabelPresent()) {
            observer.disconnect();
            resolve(true);
          }
        });
        observer.observe(document.body, { childList: true, subtree: true });
      })
    )")
                    .ExtractBool());
  }

  bool HasNewTabTakeoverInfoBar() {
    infobars::ContentInfoBarManager* const infobar_manager =
        infobars::ContentInfoBarManager::FromWebContents(
            GetActiveWebContents());
    CHECK(infobar_manager);
    return std::ranges::any_of(
        infobar_manager->infobars(), [](const infobars::InfoBar* infobar) {
          return infobar->delegate()->GetIdentifier() ==
                 infobars::InfoBarDelegate::NEW_TAB_TAKEOVER_INFOBAR_DELEGATE;
        });
  }

 protected:
  virtual std::string GetComponentDataDirectoryName() { return "static"; }

 private:
  base::test::ScopedFeatureList feature_list_;
  base::CallbackListSubscription callback_list_subscription_;
};

IN_PROC_BROWSER_TEST_F(NewTabTakeoverBrowserTest,
                       DisplayNewTabTakeoverOnNewTabPage) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });

  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(/*ad=*/nullptr));

  OpenNewTabAndWaitForLoad();
  VerifyNewTabPageLoadedExpectation();
}

IN_PROC_BROWSER_TEST_F(NewTabTakeoverBrowserTest,
                       DisplayNewTabTakeoverDisclosureForStaticNewTabTakeover) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });

  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(BuildNewTabPageAd()));

  OpenNewTabAndWaitForLoad();
  VerifyNewTabPageLoadedExpectation();
  WaitForNewTabTakeoverDisclosureLabel();
}

IN_PROC_BROWSER_TEST_F(
    NewTabTakeoverBrowserTest,
    AutoDisplayNewTabTakeoverDisclosureTooltipDecrementsRemainingCount) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });

  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(BuildNewTabPageAd()));

  PrefService* const prefs = GetProfile()->GetPrefs();
  const int initial_count =
      prefs->GetInteger(ntp_background_images::prefs::
                            kNewTabTakeoverInfobarRemainingDisplayCount);

  OpenNewTabAndWaitForLoad();
  VerifyNewTabPageLoadedExpectation();
  WaitForNewTabTakeoverDisclosureLabel();

  ASSERT_TRUE(base::test::RunUntil([&] {
    return prefs->GetInteger(ntp_background_images::prefs::
                                 kNewTabTakeoverInfobarRemainingDisplayCount) <
           initial_count;
  }));
}

IN_PROC_BROWSER_TEST_F(NewTabTakeoverBrowserTest,
                       NotDisplayNewTabTakeoverInfobarOnNewTabPage) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });

  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(BuildNewTabPageAd()));

  OpenNewTabAndWaitForLoad();
  VerifyNewTabPageLoadedExpectation();
  WaitForNewTabTakeoverDisclosureLabel();

  EXPECT_FALSE(HasNewTabTakeoverInfoBar());
}

IN_PROC_BROWSER_TEST_F(
    NewTabTakeoverBrowserTest,
    ClickNewTabTakeoverDisclosureLearnMoreSuppressesTooltipAutoDisplay) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });

  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(BuildNewTabPageAd()));

  OpenNewTabAndWaitForLoad();
  VerifyNewTabPageLoadedExpectation();
  WaitForNewTabTakeoverDisclosureLabel();

  ASSERT_TRUE(content::ExecJs(
      GetActiveWebContents(),
      "document.querySelector('.new-tab-takeover-disclosure-tooltip a')"
      ".click()"));

  PrefService* const prefs = GetProfile()->GetPrefs();
  EXPECT_TRUE(base::test::RunUntil([&] {
    return prefs->GetInteger(ntp_background_images::prefs::
                                 kNewTabTakeoverInfobarRemainingDisplayCount) ==
           0;
  }));
}

IN_PROC_BROWSER_TEST_F(NewTabTakeoverBrowserTest,
                       NotDisplayNewTabTakeoverOnRestoredNewTabPage) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });
  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(/*ad=*/nullptr));
  OpenNewTabAndWaitForLoad();
  VerifyNewTabPageLoadedExpectation();

  brave_ads::AdsServiceMock& ads_service_mock = GetAdsServiceMock();
  EXPECT_CALL(ads_service_mock, MaybeServeNewTabPageAd).Times(0);

  CloseBrowserAndRestoreSession();

  WaitForLoadStop();
  VerifyNewTabPageLoadedExpectation();
  testing::Mock::VerifyAndClearExpectations(&ads_service_mock);

  EXPECT_CALL(ads_service_mock, MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(/*ad=*/nullptr));
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
  WaitForLoadStop();
  VerifyNewTabPageLoadedExpectation();
}

class DynamicNewTabTakeoverBrowserTest : public NewTabTakeoverBrowserTest {
 protected:
  std::string GetComponentDataDirectoryName() override { return "dynamic"; }
};

IN_PROC_BROWSER_TEST_F(DynamicNewTabTakeoverBrowserTest,
                       DisplayNewTabTakeoverDisclosure) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });

  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(BuildNewTabPageAd()));

  OpenNewTabAndWaitForLoad();
  VerifyNewTabPageLoadedExpectation();
  WaitForNewTabTakeoverDisclosureLabel();
}

class NewTabTakeoverForLegacyNewTabPageBrowserTest
    : public NewTabTakeoverBrowserTest {
 public:
  NewTabTakeoverForLegacyNewTabPageBrowserTest() {
    new_tab_page_refresh_feature_list_.InitAndDisableFeature(
        features::kBraveNewTabPageRefreshEnabled);
  }

 private:
  base::test::ScopedFeatureList new_tab_page_refresh_feature_list_;
};

IN_PROC_BROWSER_TEST_F(NewTabTakeoverForLegacyNewTabPageBrowserTest,
                       DisplayNewTabTakeoverInfobar) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });

  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(BuildNewTabPageAd()));

  OpenNewTabAndWaitForLoad();

  EXPECT_TRUE(base::test::RunUntil([&] { return HasNewTabTakeoverInfoBar(); }));
}

IN_PROC_BROWSER_TEST_F(
    NewTabTakeoverForLegacyNewTabPageBrowserTest,
    DisplayNewTabTakeoverInfobarDecrementsRemainingDisplayCount) {
  ON_CALL(GetAdsServiceMock(), GetStatementOfAccounts(::testing::_))
      .WillByDefault([](brave_ads::GetStatementOfAccountsCallback callback) {
        std::move(callback).Run(/*mojom_statement=*/nullptr);
      });

  EXPECT_CALL(GetAdsServiceMock(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(BuildNewTabPageAd()));

  PrefService* const prefs = GetProfile()->GetPrefs();
  const int initial_count =
      prefs->GetInteger(ntp_background_images::prefs::
                            kNewTabTakeoverInfobarRemainingDisplayCount);

  OpenNewTabAndWaitForLoad();
  ASSERT_TRUE(base::test::RunUntil([&] { return HasNewTabTakeoverInfoBar(); }));

  EXPECT_EQ(initial_count - 1,
            prefs->GetInteger(ntp_background_images::prefs::
                                  kNewTabTakeoverInfobarRemainingDisplayCount));
}
