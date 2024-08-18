/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/debug/debugging_buildflags.h"
#include "base/feature_list.h"
#include "base/feature_override.h"
#include "base/features.h"
#include "base/logging.h"
#include "chrome/browser/browser_features.h"
#include "chrome/browser/companion/core/features.h"
#include "chrome/browser/preloading/preloading_features.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/companion/visual_query/features.h"
#include "chrome/common/privacy_budget/privacy_budget_features.h"
#include "components/aggregation_service/features.h"
#include "components/attribution_reporting/features.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/commerce/core/commerce_feature_list.h"
#include "components/compose/core/browser/compose_features.h"
#include "components/content_settings/core/common/features.h"
#include "components/heap_profiling/in_process/heap_profiler_parameters.h"
#include "components/history/core/browser/features.h"
#include "components/history_clusters/core/features.h"
#include "components/history_clusters/core/on_device_clustering_features.h"
#include "components/lens/lens_features.h"
#include "components/manta/features.h"
#include "components/metrics/metrics_features.h"
#include "components/metrics/structured/structured_metrics_features.h"
#include "components/network_time/network_time_tracker.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/page_image_service/features.h"
#include "components/page_info/core/features.h"
#include "components/performance_manager/public/features.h"
#include "components/permissions/features.h"
#include "components/plus_addresses/features.h"
#include "components/privacy_sandbox/privacy_sandbox_features.h"
#include "components/safe_browsing/core/common/features.h"
#include "components/search/ntp_features.h"
#include "components/segmentation_platform/public/features.h"
#include "components/shared_highlighting/core/common/shared_highlighting_features.h"
#include "components/signin/public/base/signin_buildflags.h"
#include "components/signin/public/base/signin_switches.h"
#include "components/subresource_filter/core/common/common_features.h"
#include "components/sync/base/features.h"
#include "components/webapps/browser/features.h"
#include "content/common/features.h"
#include "content/public/common/content_features.h"
#include "content/public/common/dips_utils.h"
#include "gpu/config/gpu_finch_features.h"
#include "media/base/media_switches.h"
#include "net/base/features.h"
#include "services/network/public/cpp/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "ui/base/ui_base_features.h"

#if BUILDFLAG(IS_ANDROID)
#include "android_webview/common/aw_features.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "components/password_manager/core/common/password_manager_features.h"
#else
#include "chrome/browser/apps/link_capturing/link_capturing_features.h"
#include "chrome/browser/enterprise/connectors/analysis/content_analysis_features.h"
#include "chrome/browser/sharing_hub/sharing_hub_features.h"
#include "components/device_signals/core/common/signals_features.h"
#include "components/translate/core/common/translate_util.h"
#include "extensions/common/extension_features.h"
#include "services/device/public/cpp/device_features.h"
#include "ui/accessibility/accessibility_features.h"
#endif

TEST(FeatureDefaultsTest, DisabledFeatures) {
  // Please, keep alphabetized
  const base::Feature* disabled_features[] = {
      &aggregation_service::kAggregationServiceMultipleCloudProviders,
#if BUILDFLAG(IS_ANDROID)
      &android_webview::features::kWebViewEnumerateDevicesCache,
      &android_webview::features::kWebViewMediaIntegrityApiBlinkExtension,
#endif
      &attribution_reporting::features::kConversionMeasurement,
      &autofill::features::test::kAutofillServerCommunication,
#if BUILDFLAG(IS_ANDROID)
      &base::features::kCollectAndroidFrameTimelineMetrics,
#endif
      &blink::features::kAdAuctionReportingWithMacroApi,
      &blink::features::kAdInterestGroupAPI,
      &blink::features::kAllowURNsInIframes,
      &blink::features::kAttributionReportingInBrowserMigration,
      &blink::features::kBackgroundResourceFetch,
      &blink::features::kBiddingAndScoringDebugReportingAPI,
      &blink::features::kBrowsingTopics,
      &blink::features::kClientHintsFormFactors,
      &blink::features::kControlledFrame,
      &blink::features::kCssSelectorFragmentAnchor,
      &blink::features::kFencedFrames,
      &blink::features::kFencedFramesM120FeaturesPart2,
      &blink::features::kFledge,
      &blink::features::kFledgeBiddingAndAuctionServer,
      &blink::features::kFledgeConsiderKAnonymity,
      &blink::features::kFledgeEnforceKAnonymity,
      &blink::features::kInterestGroupStorage,
      &blink::features::kParakeet,
      &blink::features::kPrerender2,
      &blink::features::kPrivateAggregationApi,
      &blink::features::kReduceCookieIPCs,
      &blink::features::kSharedStorageAPI,
      &blink::features::kSharedStorageAPIM118,
      &blink::features::kSharedStorageAPIM125,
      &blink::features::kSharedStorageSelectURLLimit,
      &blink::features::kSpeculationRulesPrefetchFuture,
      &blink::features::kTextFragmentAnchor,
#if BUILDFLAG(IS_ANDROID)
      &chrome::android::kAdaptiveButtonInTopToolbarCustomizationV2,
#endif
      &commerce::kCommerceAllowOnDemandBookmarkUpdates,
      &commerce::kCommerceDeveloper,
      &commerce::kCommerceMerchantViewer,
      &commerce::kCommercePriceTracking,
      &commerce::kShoppingList,
      &commerce::kShoppingPDPMetrics,
      &commerce::kRetailCoupons,
      &companion::visual_query::features::kVisualQuerySuggestions,
      &compose::features::kEnableCompose,
      &content_settings::features::kTrackingProtection3pcd,
      &content_settings::features::kUserBypassUI,
#if !BUILDFLAG(IS_ANDROID)
      &companion::features::internal::
          kCompanionEnabledByObservingExpsNavigations,
      &companion::features::internal::kSidePanelCompanion,
      &companion::features::internal::kSidePanelCompanion2,
      &enterprise_signals::features::kDeviceSignalsConsentDialog,
      &extensions_features::kExtensionManifestV2DeprecationWarning,
      &extensions_features::kExtensionsManifestV3Only,
      &features::kToolbarPinning,
#endif
      &features::kBookmarkTriggerForPrerender2,
      &features::kChromeLabs,
      &features::kChromeStructuredMetrics,
      &features::kCookieDeprecationFacilitatedTesting,
#if !BUILDFLAG(IS_ANDROID)
      &features::kDesktopPWAsLinkCapturing,
#endif
      &features::kDevToolsConsoleInsights,
      &features::kDigitalGoodsApi,
      &features::kDIPS,
      &features::kFedCm,
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
      &features::kFewerUpdateConfirmations,
#endif
#if !BUILDFLAG(IS_ANDROID)
      &features::kHaTSWebUI,
#endif
      &features::kIdentifiabilityStudyMetaExperiment,
      &features::kIdleDetection,
      &features::kKAnonymityService,
      &features::kKAnonymityServiceOHTTPRequests,
      &features::kNewTabPageTriggerForPrerender2,
      &features::kNotificationTriggers,
#if BUILDFLAG(IS_ANDROID)
      &features::kPrivacyGuidePreloadAndroid,
#endif
      &features::kPrivacySandboxAdsAPIsOverride,
      &features::kResourceTimingForCancelledNavigationInFrame,
      &features::kSCTAuditing,
      &features::kServiceWorkerAutoPreload,
      &features::kSupportSearchSuggestionForPrerender2,
      &features::kTabHoverCardImages,
#if !BUILDFLAG(IS_ANDROID)
      &features::kTrustSafetySentimentSurvey,
      &features::kTrustSafetySentimentSurveyV2,
#endif
#if BUILDFLAG(IS_MAC)
      &features::kUseChromiumUpdater,
#endif
#if !BUILDFLAG(IS_ANDROID)
      &features::kUseMoveNotCopyInMergeTreeUpdate,
      &features::kWebAppUniversalInstall,
#endif
      &features::kWebIdentityDigitalCredentials,
      &features::kWebOTP,
      &heap_profiling::kHeapProfilerReporting,
      &history::kOrganicRepeatableQueries,
      &history::kSyncSegmentsData,
      &history_clusters::kSidePanelJourneys,
      &history_clusters::features::kOnDeviceClustering,
      &history_clusters::features::kOnDeviceClusteringKeywordFiltering,
      &history_clusters::internal::kHistoryClustersInternalsPage,
      &history_clusters::internal::kHistoryClustersNavigationContextClustering,
      &history_clusters::internal::kJourneys,
      &history_clusters::internal::kJourneysImages,
      &history_clusters::internal::kJourneysNamedNewTabGroups,
      &history_clusters::internal::kJourneysPersistCachesToPrefs,
      &history_clusters::internal::kJourneysZeroStateFiltering,
      &history_clusters::internal::kOmniboxAction,
      &history_clusters::internal::kOmniboxHistoryClusterProvider,
      &history_clusters::internal::kPersistContextAnnotationsInHistoryDb,
#if BUILDFLAG(ENABLE_MIRROR)
      &kVerifyRequestInitiatorForMirrorHeaders,
#endif
      &lens::features::kLensStandalone,
      &manta::features::kMantaService,
      &media::kLiveCaption,
      &metrics::features::kMetricsServiceDeltaSnapshotInBg,
      &metrics::structured::kEnabledStructuredMetricsService,
      &metrics::structured::kPhoneHubStructuredMetrics,
      &net::features::kEnableWebTransportDraft07,
      &net::features::kTopLevelTpcdOriginTrial,
      &net::features::kTpcdMetadataGrants,
      &net::features::kWaitForFirstPartySetsInit,
      &network::features::kFledgePst,
      &network::features::kPrivateStateTokens,
      &network_time::kNetworkTimeServiceQuerying,
      &ntp_features::kCustomizeChromeSidePanelExtensionsCard,
      &ntp_features::kCustomizeChromeWallpaperSearch,
      &ntp_features::kNtpAlphaBackgroundCollections,
      &ntp_features::kNtpBackgroundImageErrorDetection,
      &ntp_features::kNtpChromeCartModule,
      &ntp_features::kNtpHistoryClustersModule,
      &ntp_features::kNtpHistoryClustersModuleLoad,
      &omnibox::kDocumentProviderNoSetting,
      &omnibox::kDocumentProviderNoSyncRequirement,
      &omnibox::kExpandedStateHeight,
      &omnibox::kExpandedStateShape,
      &omnibox::kMlUrlScoring,
      &omnibox::kOmniboxSteadyStateHeight,
      &omnibox::kRichAutocompletion,
      &omnibox::kStarterPackExpansion,
      &omnibox::kZeroSuggestPrefetching,
      &optimization_guide::features::kOptimizationGuideFetchingForSRP,
      &optimization_guide::features::kOptimizationGuidePersonalizedFetching,
      &optimization_guide::features::kOptimizationHints,
      &optimization_guide::features::kRemoteOptimizationGuideFetching,
      &optimization_guide::features::
          kRemoteOptimizationGuideFetchingAnonymousDataConsent,
      &page_image_service::kImageService,
      &page_image_service::kImageServiceSuggestPoweredImages,
#if BUILDFLAG(IS_ANDROID)
      &password_manager::features::
          kUnifiedPasswordManagerLocalPasswordsMigrationWarning,
#endif
#if !BUILDFLAG(IS_ANDROID)
      &permissions::features::kPermissionsPromptSurvey,
#endif
      &permissions::features::kPermissionOnDeviceNotificationPredictions,
      &permissions::features::kShowRelatedWebsiteSetsPermissionGrants,
      &plus_addresses::features::kPlusAddressesEnabled,
      &privacy_sandbox::kEnforcePrivacySandboxAttestations,
      &privacy_sandbox::kOverridePrivacySandboxSettingsLocalTesting,
      &privacy_sandbox::kPrivacySandboxFirstPartySetsUI,
      &privacy_sandbox::kPrivacySandboxProactiveTopicsBlocking,
      &privacy_sandbox::kPrivacySandboxSettings4,
      &privacy_sandbox::kTrackingProtectionContentSettingUbControl,
      &safe_browsing::kExtensionTelemetryDisableOffstoreExtensions,
      &safe_browsing::kExtensionTelemetryForEnterprise,
      &safe_browsing::kExtensionTelemetryTabsApiSignal,
      &safe_browsing::kGooglePlayProtectInApkTelemetry,
      &segmentation_platform::features::kSegmentationPlatformCollectTabRankData,
      &segmentation_platform::features::kSegmentationPlatformDeviceTier,
      &segmentation_platform::features::kSegmentationPlatformFeature,
      &segmentation_platform::features::kSegmentationPlatformTimeDelaySampling,
      &shared_highlighting::kIOSSharedHighlightingV2,
      &shared_highlighting::kSharedHighlightingManager,
      &subresource_filter::kAdTagging,
      &syncer::kSyncEnableBookmarksInTransportMode,
#if !BUILDFLAG(IS_ANDROID)
      &translate::kTFLiteLanguageDetectionEnabled,
#endif
      &webapps::features::kWebAppsEnableMLModelForPromotion,
  };

  for (const auto* feature : disabled_features) {
    EXPECT_FALSE(base::FeatureList::IsEnabled(*feature)) << feature->name;
  }
}

TEST(FeatureDefaultsTest, EnabledFeatures) {
  const base::Feature* enabled_features[] = {
      &autofill::features::kAutofillDisableShadowHeuristics,
      &blink::features::kPrefetchPrivacyChanges,
      &blink::features::kReducedReferrerGranularity,
      &blink::features::kReduceUserAgentMinorVersion,
      &blink::features::kUACHOverrideBlank,
      &features::kCertificateTransparencyAskBeforeEnabling,
#if !BUILDFLAG(IS_ANDROID)
      &features::kLocationProviderManager,
#endif
      &media::kEnableTabMuting,
      &net::features::kPartitionConnectionsByNetworkIsolationKey,
#if !BUILDFLAG(IS_ANDROID)
      &sharing_hub::kDesktopScreenshots,
#endif
  };

  for (const auto* feature : enabled_features) {
    EXPECT_TRUE(base::FeatureList::IsEnabled(*feature)) << feature->name;
  }
}

TEST(FeatureDefaultsTest, DefaultFeatureParameters) {
#if !BUILDFLAG(IS_ANDROID)
  EXPECT_EQ(features::kLocationProviderManagerParam.default_value,
            device::mojom::LocationProviderManagerMode::kPlatformOnly);
#endif
}
