/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>

#include "base/callback_list.h"
#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/browser/brave_ads/ad_units/notification_ad/notification_ad_platform_bridge.h"
#include "brave/browser/brave_ads/ads_service_delegate.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/ads_service_waiter.h"
#include "brave/browser/brave_ads/device_id/device_id_impl.h"
#include "brave/browser/brave_ads/services/bat_ads_service_factory_impl.h"
#include "brave/browser/brave_ads/virtual_pref_provider_delegate.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "brave/components/brave_ads/core/browser/network/http_client.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_util.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_ads/tooltips/ads_tooltips_delegate_impl.h"
#endif

// npm run test -- brave_browser_tests --filter=BraveAdsServiceImpl*

namespace brave_ads {

namespace {

network::mojom::NetworkContext* GetNetworkContext(
    content::BrowserContext* context) {
  return context->GetDefaultStoragePartition()->GetNetworkContext();
}

std::unique_ptr<KeyedService> CreateRealAdsService(
    content::BrowserContext* context) {
  auto* profile = Profile::FromBrowserContext(context);

  auto* prefs = profile->GetPrefs();
  auto* local_state = g_browser_process->local_state();

  auto* brave_adaptive_captcha_service =
      brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetForProfile(
          profile);
  CHECK(brave_adaptive_captcha_service);

  auto delegate = std::make_unique<AdsServiceDelegate>(
      *profile, local_state, *brave_adaptive_captcha_service,
      std::make_unique<NotificationAdPlatformBridge>(*profile));

  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  auto* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
#endif

  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);

  auto http_client = std::make_unique<HttpClient>(
      *local_state,
      profile->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess(),
      base::BindRepeating(&GetNetworkContext, context),
      /*use_ohttp_staging=*/IsStagingEnvironment(*prefs));

#if BUILDFLAG(IS_ANDROID)
  std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate;
#else
  auto ads_tooltips_delegate = std::make_unique<AdsTooltipsDelegateImpl>();
#endif

  return std::make_unique<AdsServiceImpl>(
      std::move(delegate), prefs, local_state, std::move(http_client),
      std::make_unique<VirtualPrefProviderDelegate>(*profile),
      brave::GetChannelName(), profile->GetPath(),
      std::move(ads_tooltips_delegate), std::make_unique<DeviceIdImpl>(),
      std::make_unique<BatAdsServiceFactoryImpl>(),
      g_brave_browser_process->resource_component(), history_service,
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
      rewards_service,
#endif
      host_content_settings_map);
}

}  // namespace

// Tests exercise the real AdsServiceImpl with bat-ads not running (rewards not
<<<<<<< Updated upstream
// enabled). In this state methods immediately call their callbacks with
// failure/empty values, or perform synchronous cleanup (ClearData).
=======
// enabled). Methods that proxy to bat-ads immediately call their callbacks with
// failure/empty values, while ClearData performs its file cleanup and notifies
// observers.
>>>>>>> Stashed changes
class BraveAdsServiceImplTest : public PlatformBrowserTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();

    callback_list_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(base::BindRepeating(
                &BraveAdsServiceImplTest::OnWillCreateBrowserContextServices,
                base::Unretained(this)));
  }

  void TearDownInProcessBrowserTestFixture() override {
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void OnWillCreateBrowserContextServices(
      content::BrowserContext* const context) {
    AdsServiceFactory::GetInstance()->SetTestingFactory(
        context, base::BindRepeating(&CreateRealAdsService));
  }

  AdsService* GetAdsService() {
    return AdsServiceFactory::GetForProfile(GetProfile());
  }

  Profile* GetProfile() { return chrome_test_utils::GetProfile(this); }
<<<<<<< Updated upstream
=======

 private:
  base::CallbackListSubscription callback_list_subscription_;
>>>>>>> Stashed changes
};

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       GetDiagnosticsWithServiceNotInitialized) {
  base::MockOnceCallback<void(std::optional<base::ListValue>)> callback;
  EXPECT_CALL(callback, Run(std::optional<base::ListValue>(std::nullopt)));

  GetAdsService()->GetDiagnostics(callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       GetStatementOfAccountsWithServiceNotInitialized) {
  base::MockOnceCallback<void(mojom::StatementInfoPtr)> callback;
<<<<<<< Updated upstream
  EXPECT_CALL(callback, Run(testing::IsNull()));
=======
  EXPECT_CALL(callback, Run(testing::Truly(
                               [](const mojom::StatementInfoPtr& statement) {
                                 return !statement;
                               })));
>>>>>>> Stashed changes

  GetAdsService()->GetStatementOfAccounts(callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       GetInternalsWithServiceNotInitialized) {
  base::MockOnceCallback<void(std::optional<base::DictValue>)> callback;
  EXPECT_CALL(callback, Run(std::optional<base::DictValue>(std::nullopt)));

  GetAdsService()->GetInternals(callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       GetAdHistoryWithServiceNotInitialized) {
  base::MockOnceCallback<void(std::optional<base::ListValue>)> callback;
  EXPECT_CALL(callback, Run(std::optional<base::ListValue>(std::nullopt)));

  GetAdsService()->GetAdHistory(base::Time::Now() - base::Days(7),
                                base::Time::Now(), callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       ToggleLikeAdWithServiceNotInitialized) {
  base::MockOnceCallback<void(bool)> callback;
  EXPECT_CALL(callback, Run(false));

  GetAdsService()->ToggleLikeAd(mojom::ReactionInfo::New(), callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       ToggleDislikeAdWithServiceNotInitialized) {
  base::MockOnceCallback<void(bool)> callback;
  EXPECT_CALL(callback, Run(false));

  GetAdsService()->ToggleDislikeAd(mojom::ReactionInfo::New(), callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       ToggleLikeSegmentWithServiceNotInitialized) {
  base::MockOnceCallback<void(bool)> callback;
  EXPECT_CALL(callback, Run(false));

  GetAdsService()->ToggleLikeSegment(mojom::ReactionInfo::New(),
                                     callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       ToggleDislikeSegmentWithServiceNotInitialized) {
  base::MockOnceCallback<void(bool)> callback;
  EXPECT_CALL(callback, Run(false));

  GetAdsService()->ToggleDislikeSegment(mojom::ReactionInfo::New(),
                                        callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       ToggleSaveAdWithServiceNotInitialized) {
  base::MockOnceCallback<void(bool)> callback;
  EXPECT_CALL(callback, Run(false));

  GetAdsService()->ToggleSaveAd(mojom::ReactionInfo::New(), callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest,
                       ToggleMarkAdAsInappropriateWithServiceNotInitialized) {
  base::MockOnceCallback<void(bool)> callback;
  EXPECT_CALL(callback, Run(false));

  GetAdsService()->ToggleMarkAdAsInappropriate(mojom::ReactionInfo::New(),
                                               callback.Get());
}

IN_PROC_BROWSER_TEST_F(BraveAdsServiceImplTest, ClearData) {
  // ClearData shuts down bat-ads (already stopped), clears all "brave.brave_ads"
  // prefs, deletes the ads_service directory on the file thread, notifies
<<<<<<< Updated upstream
  // observers, then calls the callback with success=true.
=======
  // observers via OnDidClearAdsServiceData, then calls the callback with true.
>>>>>>> Stashed changes
  test::AdsServiceWaiter waiter(GetAdsService());

  base::RunLoop run_loop;
  base::MockOnceCallback<void(bool)> callback;
  EXPECT_CALL(callback, Run(true))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));

  GetAdsService()->ClearData(callback.Get());

  waiter.WaitForOnDidClearAdsServiceData();
  run_loop.Run();
}

}  // namespace brave_ads
