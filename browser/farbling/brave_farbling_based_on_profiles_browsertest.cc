// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_shields/brave_farbling_service_factory.h"
#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

class BraveFarblingBasedOnProfilesBrowserTest : public InProcessBrowserTest {};

IN_PROC_BROWSER_TEST_F(BraveFarblingBasedOnProfilesBrowserTest,
                       CheckBetweenNormalAndIncognitoProfile) {
  auto* profile1 = browser()->profile();
  auto* incognito_profile = CreateIncognitoBrowser(profile1)->profile();

  auto* brave_farbling_service =
      brave::BraveFarblingServiceFactory::GetForProfile(profile1);

  auto* brave_farbling_service_incognito =
      brave::BraveFarblingServiceFactory::GetForProfile(incognito_profile);

  CHECK(brave_farbling_service);
  CHECK(brave_farbling_service_incognito);

  EXPECT_NE(brave_farbling_service->session_token(),
            brave_farbling_service_incognito->session_token());
}

IN_PROC_BROWSER_TEST_F(BraveFarblingBasedOnProfilesBrowserTest,
                       CheckBetweenTwoProfiles) {
  auto* profile_1 = browser()->profile();
  CHECK(profile_1);

  // Create another profile.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);
  base::FilePath dest_path = profile_manager->user_data_dir();
  dest_path = dest_path.Append(FILE_PATH_LITERAL("Profile2"));
  Profile* profile_2 = nullptr;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    profile_2 = profile_manager->GetProfile(dest_path);
  }
  CHECK(profile_2);
  auto* browser_2 = CreateBrowser(profile_2);
  CHECK(browser_2);

  auto* brave_farbling_service_profile_1 =
      brave::BraveFarblingServiceFactory::GetForProfile(profile_1);
  CHECK(brave_farbling_service_profile_1);

  auto* brave_farbling_service_profile_2 =
      brave::BraveFarblingServiceFactory::GetForProfile(profile_2);
  CHECK(brave_farbling_service_profile_2);

  EXPECT_NE(brave_farbling_service_profile_1->session_token(),
            brave_farbling_service_profile_2->session_token());
}
