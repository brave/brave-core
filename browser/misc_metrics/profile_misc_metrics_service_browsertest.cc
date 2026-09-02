/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"

#include "base/files/file_path.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/components/misc_metrics/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/profile_waiter.h"
#include "components/keyed_service/core/keyed_service_base_factory.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

namespace misc_metrics {

class ProfileMiscMetricsServiceBrowserTest : public InProcessBrowserTest {
 public:
  ProfileMiscMetricsServiceBrowserTest() {
    // Disabled by default, which would make the fingerprint metrics
    // expectations below vacuous.
    feature_list_.InitAndEnableFeature(features::kFingerprintInputMetrics);
  }

 protected:
  // Returns the on-the-record profile at `profile_path`, once all of its
  // initialization tasks have run.
  Profile* CreateProfileAndWaitForAllTasks(const base::FilePath& profile_path) {
    ProfileWaiter profile_waiter;
    g_browser_process->profile_manager()->CreateProfileAsync(profile_path, {});
    Profile* profile = profile_waiter.WaitForProfileAdded();
    content::RunAllTasksUntilIdle();
    return profile;
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(ProfileMiscMetricsServiceBrowserTest,
                       RegularProfileHasFingerprintMetrics) {
  auto* service = ProfileMiscMetricsServiceFactory::GetServiceForContext(
      browser()->GetProfile());
  ASSERT_TRUE(service);
  EXPECT_TRUE(service->GetFingerprintFrequencyMetricsForTesting());
}

// Keyed services are disabled by default for the System Profile, so navigating
// in it crashes throttles that dereference services it doesn't have. The
// metrics collector navigates a WebContents of its own, so nothing here may be
// created for that profile.
IN_PROC_BROWSER_TEST_F(ProfileMiscMetricsServiceBrowserTest,
                       SystemProfileHasNoMiscMetricsService) {
  Profile* system_profile =
      CreateProfileAndWaitForAllTasks(ProfileManager::GetSystemProfilePath());
  ASSERT_FALSE(system_profile->IsOffTheRecord());
  ASSERT_TRUE(system_profile->IsSystemProfile());

  // `IsServiceCreated` is only public on the base factory.
  KeyedServiceBaseFactory* factory =
      ProfileMiscMetricsServiceFactory::GetInstance();
  EXPECT_FALSE(factory->IsServiceCreated(system_profile));
}

IN_PROC_BROWSER_TEST_F(ProfileMiscMetricsServiceBrowserTest,
                       GuestProfileHasNoFingerprintMetrics) {
  Browser* guest_browser = CreateGuestBrowser();
  Profile* guest_profile = guest_browser->GetProfile()->GetOriginalProfile();
  content::RunAllTasksUntilIdle();
  ASSERT_FALSE(guest_profile->IsOffTheRecord());
  ASSERT_TRUE(guest_profile->IsGuestSession());

  auto* service =
      ProfileMiscMetricsServiceFactory::GetServiceForContext(guest_profile);
  ASSERT_TRUE(service);
  EXPECT_FALSE(service->GetFingerprintFrequencyMetricsForTesting());
}

}  // namespace misc_metrics
