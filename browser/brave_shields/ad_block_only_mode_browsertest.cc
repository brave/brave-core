/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/debounce/core/common/pref_names.h"
#include "brave/components/global_privacy_control/pref_names.h"
#include "brave/components/query_filter/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

class AdblockOnlyModeChangeObserver final {
 public:
  explicit AdblockOnlyModeChangeObserver(PrefService* prefs) {
    pref_change_registrar_.Init(prefs);
    // Wait for a single pref change since all prefs are updated simultaneously.
    pref_change_registrar_.Add(prefs::kReduceLanguageEnabled,
                               run_loop_.QuitClosure());
  }
  ~AdblockOnlyModeChangeObserver() = default;

  void Wait() { run_loop_.Run(); }

 private:
  PrefChangeRegistrar pref_change_registrar_;
  base::RunLoop run_loop_;
};

}  // namespace

class AdblockOnlyModeBrowserTestBase : public PlatformBrowserTest {
 public:
  AdblockOnlyModeBrowserTestBase() = default;
  ~AdblockOnlyModeBrowserTestBase() override = default;

  PrefService* local_state() { return g_browser_process->local_state(); }

  PrefService* profile_prefs() { return GetProfile()->GetPrefs(); }

  brave_shields::BraveShieldsSettingsService* brave_shields_settings() {
    return BraveShieldsSettingsServiceFactory::GetForProfile(GetProfile());
  }

  // Setup settings causing web compat issues to verify that Adblock Only Mode
  // overrides them.
  void SetupSettingsCausingWebCompatIssues() {
    // Block JavaScript.
    SetNoScriptControlType(host_content_settings_map(), ControlType::BLOCK,
                           GURL());

    // Block third-party cookies.
    SetCookieControlType(host_content_settings_map(), profile_prefs(),
                         ControlType::BLOCK_THIRD_PARTY, GURL(), nullptr);

    // Enable Brave Fingerprinting protection.
    SetFingerprintingControlType(host_content_settings_map(),
                                 ControlType::DEFAULT, GURL());

    // Set `Upgrade connections to HTTPS` to require HTTPS.
    SetHttpsUpgradeControlType(host_content_settings_map(), ControlType::BLOCK,
                               GURL());

    // Set forget first-party storage to enabled.
    brave_shields_settings()->SetForgetFirstPartyStorageEnabled(true, GURL());

    // Enable language fingerprinting reduction.
    profile_prefs()->SetBoolean(prefs::kReduceLanguageEnabled, true);

    // Enable De-AMP.
    profile_prefs()->SetBoolean(de_amp::kDeAmpPrefEnabled, true);

    // Enable URL debouncing.
    profile_prefs()->SetBoolean(debounce::prefs::kDebounceEnabled, true);
  }

  void VerifySettingsCausingWebCompatIssues() {
    // Verify that JavaScript is blocked.
    EXPECT_EQ(GetNoScriptControlType(host_content_settings_map(), GURL()),
              ControlType::BLOCK);

    // Verify that third-party cookies are blocked.
    EXPECT_EQ(GetCookieControlType(host_content_settings_map(),
                                   cookie_settings().get(), GURL()),
              ControlType::BLOCK_THIRD_PARTY);

    // Verify that Brave Fingerprinting protection is enabled.
    EXPECT_EQ(GetFingerprintingControlType(host_content_settings_map(), GURL()),
              ControlType::DEFAULT);

    // Verify that `Upgrade connections to HTTPS` is set to require HTTPS.
    EXPECT_EQ(GetHttpsUpgradeControlType(host_content_settings_map(), GURL()),
              ControlType::BLOCK);

    // Verify that referrer policy is capped.
    EXPECT_FALSE(brave_shields::AreReferrersAllowed(host_content_settings_map(),
                                                    GURL()));

    // Verify that forget first-party storage is enabled.
    EXPECT_TRUE(
        brave_shields_settings()->GetForgetFirstPartyStorageEnabled(GURL()));

    // Verify that language fingerprinting reduction is enabled and not managed.
    EXPECT_TRUE(profile_prefs()->GetBoolean(prefs::kReduceLanguageEnabled));
    EXPECT_FALSE(
        profile_prefs()->IsManagedPreference(prefs::kReduceLanguageEnabled));

    // Verify that De-AMP is enabled and not managed.
    EXPECT_TRUE(profile_prefs()->GetBoolean(de_amp::kDeAmpPrefEnabled));
    EXPECT_FALSE(
        profile_prefs()->IsManagedPreference(de_amp::kDeAmpPrefEnabled));

    // Verify that URL debouncing is enabled and not managed.
    EXPECT_TRUE(profile_prefs()->GetBoolean(debounce::prefs::kDebounceEnabled));
    EXPECT_FALSE(profile_prefs()->IsManagedPreference(
        debounce::prefs::kDebounceEnabled));

    // Verify that tracking query parameters filtering is enabled and not
    // managed.
    EXPECT_TRUE(profile_prefs()->GetBoolean(
        query_filter::kTrackingQueryParametersFilteringEnabled));
    EXPECT_FALSE(profile_prefs()->IsManagedPreference(
        query_filter::kTrackingQueryParametersFilteringEnabled));

    // Verify that Global Privacy Control is enabled and not managed.
    EXPECT_TRUE(profile_prefs()->GetBoolean(
        global_privacy_control::kGlobalPrivacyControlEnabled));
    EXPECT_FALSE(profile_prefs()->IsManagedPreference(
        global_privacy_control::kGlobalPrivacyControlEnabled));
  }

  void VerifyAdblockOnlyModeSettings() {
    // Verify that JavaScript is allowed.
    EXPECT_EQ(GetNoScriptControlType(host_content_settings_map(), GURL()),
              ControlType::ALLOW);

    // Verify that cookies are allowed.
    EXPECT_EQ(GetCookieControlType(host_content_settings_map(),
                                   cookie_settings().get(), GURL()),
              ControlType::ALLOW);

    // Verify that Brave Fingerprinting protection is disabled.
    EXPECT_EQ(GetFingerprintingControlType(host_content_settings_map(), GURL()),
              ControlType::ALLOW);

    // Verify that `Upgrade connections to HTTPS` is set to standard mode.
    EXPECT_EQ(GetHttpsUpgradeControlType(host_content_settings_map(), GURL()),
              ControlType::BLOCK_THIRD_PARTY);

    // Verify that referrers are not capped.
    EXPECT_TRUE(brave_shields::AreReferrersAllowed(host_content_settings_map(),
                                                   GURL()));

    // Verify that forget first-party storage is disabled.
    EXPECT_FALSE(
        brave_shields_settings()->GetForgetFirstPartyStorageEnabled(GURL()));

    // Verify that language fingerprinting reduction is disabled and managed.
    EXPECT_FALSE(profile_prefs()->GetBoolean(prefs::kReduceLanguageEnabled));
    EXPECT_TRUE(
        profile_prefs()->IsManagedPreference(prefs::kReduceLanguageEnabled));

    // Verify that De-AMP is disabled and managed.
    EXPECT_FALSE(profile_prefs()->GetBoolean(de_amp::kDeAmpPrefEnabled));
    EXPECT_TRUE(
        profile_prefs()->IsManagedPreference(de_amp::kDeAmpPrefEnabled));

    // Verify that URL debouncing is disabled and managed.
    EXPECT_FALSE(
        profile_prefs()->GetBoolean(debounce::prefs::kDebounceEnabled));
    EXPECT_TRUE(profile_prefs()->IsManagedPreference(
        debounce::prefs::kDebounceEnabled));

    // Verify that tracking query parameters filtering is disabled and managed.
    EXPECT_FALSE(profile_prefs()->GetBoolean(
        query_filter::kTrackingQueryParametersFilteringEnabled));
    EXPECT_TRUE(profile_prefs()->IsManagedPreference(
        query_filter::kTrackingQueryParametersFilteringEnabled));

    // Verify that Global Privacy Control is disabled and managed.
    EXPECT_FALSE(profile_prefs()->GetBoolean(
        global_privacy_control::kGlobalPrivacyControlEnabled));
    EXPECT_TRUE(profile_prefs()->IsManagedPreference(
        global_privacy_control::kGlobalPrivacyControlEnabled));
  }

 private:
  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(GetProfile());
  }

  scoped_refptr<content_settings::CookieSettings> cookie_settings() {
    return CookieSettingsFactory::GetForProfile(GetProfile());
  }
};

class AdblockOnlyModeBrowserTest : public AdblockOnlyModeBrowserTestBase {
 public:
  AdblockOnlyModeBrowserTest() = default;
  ~AdblockOnlyModeBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_{
      features::kAdblockOnlyMode};
};

IN_PROC_BROWSER_TEST_F(AdblockOnlyModeBrowserTest,
                       AdblockOnlyModeOverridesSettings) {
  EXPECT_FALSE(local_state()->GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  SetupSettingsCausingWebCompatIssues();

  AdblockOnlyModeChangeObserver observer(profile_prefs());
  local_state()->SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  observer.Wait();

  VerifyAdblockOnlyModeSettings();
}

IN_PROC_BROWSER_TEST_F(
    AdblockOnlyModeBrowserTest,
    PRE_AdblockOnlyModeOverridesSettingsAfterBrowserRestart) {
  EXPECT_FALSE(local_state()->GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  SetupSettingsCausingWebCompatIssues();

  local_state()->SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
}

IN_PROC_BROWSER_TEST_F(AdblockOnlyModeBrowserTest,
                       AdblockOnlyModeOverridesSettingsAfterBrowserRestart) {
  VerifyAdblockOnlyModeSettings();
}

IN_PROC_BROWSER_TEST_F(AdblockOnlyModeBrowserTest,
                       PRE_AdblockOnlyModeDisablingRestoresSettings) {
  EXPECT_FALSE(local_state()->GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  SetupSettingsCausingWebCompatIssues();

  local_state()->SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
}

IN_PROC_BROWSER_TEST_F(AdblockOnlyModeBrowserTest,
                       AdblockOnlyModeDisablingRestoresSettings) {
  VerifyAdblockOnlyModeSettings();

  AdblockOnlyModeChangeObserver observer(profile_prefs());
  local_state()->SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
  observer.Wait();

  VerifySettingsCausingWebCompatIssues();
}

class AdblockOnlyModeFeatureDisabledBrowserTest
    : public AdblockOnlyModeBrowserTestBase {
 public:
  AdblockOnlyModeFeatureDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(features::kAdblockOnlyMode);
  }

  ~AdblockOnlyModeFeatureDisabledBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(AdblockOnlyModeFeatureDisabledBrowserTest,
                       PRE_AdblockOnlyModeCannotBeEnabled) {
  EXPECT_FALSE(local_state()->GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  SetupSettingsCausingWebCompatIssues();

  local_state()->SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
}

IN_PROC_BROWSER_TEST_F(AdblockOnlyModeFeatureDisabledBrowserTest,
                       AdblockOnlyModeCannotBeEnabled) {
  // Adblock only mode is not applied because the feature is not enabled.
  VerifySettingsCausingWebCompatIssues();
}

}  // namespace brave_shields
