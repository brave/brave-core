/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/profiles/profile_impl.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/hats/hats_service.h"
#include "chrome/browser/ui/hats/hats_service_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// Simplified version of the upstream tests modified to reflect the change in
// the HatsService chromium_src override.

namespace {

class ScopedSetMetricsConsent {
 public:
  // Enables or disables metrics consent based off of |consent|.
  explicit ScopedSetMetricsConsent(bool consent) : consent_(consent) {
    ChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(
        &consent_);
  }

  ScopedSetMetricsConsent(const ScopedSetMetricsConsent&) = delete;
  ScopedSetMetricsConsent& operator=(const ScopedSetMetricsConsent&) = delete;

  ~ScopedSetMetricsConsent() {
    ChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(
        nullptr);
  }

 private:
  const bool consent_;
};

}  // namespace

class HatsServiceBrowserTestBase : public InProcessBrowserTest {
 protected:
  HatsServiceBrowserTestBase(const base::Feature& feature,
                             const base::FieldTrialParams& feature_parameters) {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(feature,
                                                            feature_parameters);
  }

  HatsServiceBrowserTestBase() = default;

  HatsServiceBrowserTestBase(const HatsServiceBrowserTestBase&) = delete;
  HatsServiceBrowserTestBase& operator=(const HatsServiceBrowserTestBase&) =
      delete;

  ~HatsServiceBrowserTestBase() override = default;

  HatsService* GetHatsService() {
    HatsService* service =
        HatsServiceFactory::GetForProfile(browser()->profile(), true);
    return service;
  }

  void SetMetricsConsent(bool consent) {
    scoped_metrics_consent_.emplace(consent);
  }

  bool HatsNextDialogCreated() {
    return GetHatsService()->hats_next_dialog_exists_for_testing();
  }

 private:
  absl::optional<ScopedSetMetricsConsent> scoped_metrics_consent_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

class HatsServiceProbabilityOne : public HatsServiceBrowserTestBase {
 public:
  HatsServiceProbabilityOne(const HatsServiceProbabilityOne&) = delete;
  HatsServiceProbabilityOne& operator=(const HatsServiceProbabilityOne&) =
      delete;

 protected:
  HatsServiceProbabilityOne()
      : HatsServiceBrowserTestBase(
            features::kHappinessTrackingSurveysForDesktopSettings,
            {{"probability", "1.000"},
             {"survey", kHatsSurveyTriggerSettings},
             {"en_site_id", "test_site_id"}}) {}

  ~HatsServiceProbabilityOne() override = default;

 private:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    // Set the profile creation time to be old enough to ensure triggering.
    browser()->profile()->SetCreationTimeForTesting(base::Time::Now() -
                                                    base::Days(45));
  }

  void TearDownOnMainThread() override {
    GetHatsService()->SetSurveyMetadataForTesting({});
  }
};

IN_PROC_BROWSER_TEST_F(HatsServiceBrowserTestBase, BubbleNotShownOnDefault) {
  GetHatsService()->LaunchSurvey(kHatsSurveyTriggerSettings);
  EXPECT_FALSE(HatsNextDialogCreated());
}

IN_PROC_BROWSER_TEST_F(HatsServiceProbabilityOne,
                       BubbleNotShownOnShowingConditionsMet) {
  SetMetricsConsent(true);
  ASSERT_TRUE(
      g_browser_process->GetMetricsServicesManager()->IsMetricsConsentGiven());
  GetHatsService()->LaunchSurvey(kHatsSurveyTriggerSettings);
  EXPECT_FALSE(HatsNextDialogCreated());
}
