/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/search_engine_provider_service.h"
#include "brave/browser/search_engines/search_engine_provider_service_factory.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/frame/brave_browser_frame.h"
#include "brave/browser/ui/views/search_engines/active_window_search_provider_manager.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

class SearchEngineProviderServiceTest : public InProcessBrowserTest {
 public:
  void SimulateWindowActivated(Browser* browser, bool active) {
    BraveBrowserFrame* frame = static_cast<BraveBrowserFrame*>(
        BrowserView::GetBrowserViewForBrowser(browser)->frame());
    frame->search_provider_manager_->OnWidgetActivationChanged(nullptr, active);
  }
};

// Check normal and private window's default provider.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       PrivateWindowPrefTestWithNonQwantRegion) {
  Profile* profile = browser()->profile();
  auto* private_browser = CreateIncognitoBrowser();

  // This test case is only for non-qwant region.
  if (brave::IsRegionForQwant(profile))
    return;

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);

  // simulate normal window is activated.
  SimulateWindowActivated(browser(), true);
  base::string16 normal_search_engine =
      service->GetDefaultSearchProvider()->data().short_name();

  // Simulate private window is activated
  SimulateWindowActivated(browser(), false);
  SimulateWindowActivated(private_browser, true);

  // Test pref is initially disabled and current provider is same with normal
  // search engine.
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(profile));
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            normal_search_engine);

  // Toggle pref and check current provideer is duckduckgo.
  brave::ToggleUseAlternativeSearchEngineProvider(profile);
  EXPECT_TRUE(brave::UseAlternativeSearchEngineProviderEnabled(profile));
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            base::ASCIIToUTF16("DuckDuckGo"));

  // Toggle pref again and check current provider is normal search engine.
  brave::ToggleUseAlternativeSearchEngineProvider(profile);
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(profile));
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            normal_search_engine);
}

#if BUILDFLAG(ENABLE_TOR)
// Check normal and tor private window's default provider.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       CheckTorWindowSearchProviderTest) {
  Profile* profile = browser()->profile();
  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  SimulateWindowActivated(browser(), true);
  const int default_normal_provider_id =
      service->GetDefaultSearchProvider()->data().prepopulate_id;

  brave::NewOffTheRecordWindowTor(browser());
  content::RunAllTasksUntilIdle();

  Browser* tor_browser = BrowserList::GetInstance()->GetLastActive();
  tor_browser->window()->Show();
  Profile* tor_profile = tor_browser->profile();
  EXPECT_TRUE(tor_profile->IsTor());

  int default_tor_provider_id =
      brave::IsRegionForQwant(tor_profile)
          ? TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT
          : TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO;

  // Activate tor window.
  SimulateWindowActivated(browser(), false);
  SimulateWindowActivated(tor_browser, true);

  // Check tor profile's search provider is set to ddg.
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().prepopulate_id,
            default_tor_provider_id);

  // Activate normal window.
  SimulateWindowActivated(tor_browser, false);
  SimulateWindowActivated(browser(), true);

  EXPECT_EQ(service->GetDefaultSearchProvider()->data().prepopulate_id,
            default_normal_provider_id);
}
#endif

// Check ddg toggle button state is changed by user's settings change.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       GuestWindowDDGToggletatusTest) {
  profiles::SwitchToGuestProfile(ProfileManager::CreateCallback());
  content::RunAllTasksUntilIdle();

  Profile* guest_profile =
      BrowserList::GetInstance()->GetLastActive()->profile();
  EXPECT_TRUE(guest_profile->IsGuestSession());

  // Guest window controller is only used in non Qwant region.
  if (brave::IsRegionForQwant(guest_profile))
    return;

  auto* template_service =
      TemplateURLServiceFactory::GetForProfile(guest_profile);

  // alternative pref is initially disabled.
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(guest_profile));

  brave::ToggleUseAlternativeSearchEngineProvider(guest_profile);
  EXPECT_TRUE(brave::UseAlternativeSearchEngineProviderEnabled(guest_profile));
  int provider_id =
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO;

  // Check guest profile's search provider is set to ddg.
  EXPECT_EQ(template_service->GetDefaultSearchProvider()->data().prepopulate_id,
            provider_id);

  auto bing_data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      guest_profile->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING);
  TemplateURL bing_url(*bing_data);
  template_service->SetUserSelectedDefaultSearchProvider(&bing_url);

  // Check alternative pref is turned off.
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(guest_profile));

  // Check alternative pref is turned on after setting ddg as a default.
  auto ddg_data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      guest_profile->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO);
  TemplateURL ddg_url(*ddg_data);

  template_service->SetUserSelectedDefaultSearchProvider(&ddg_url);
  EXPECT_TRUE(brave::UseAlternativeSearchEngineProviderEnabled(guest_profile));
}
