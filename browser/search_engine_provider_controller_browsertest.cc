/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/search_engine_provider_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"
#include "content/public/test/test_utils.h"

using SearchEngineProviderControllerTest = InProcessBrowserTest;

class TorSearchEngineProviderControllerTest : public InProcessBrowserTest {
 public:
  void SetUp() override {
    disable_io_checks();
    InProcessBrowserTest::SetUp();
  }
};

TemplateURLData CreateTestSearchEngine() {
  TemplateURLData result;
  result.SetShortName(base::ASCIIToUTF16("test1"));
  result.SetKeyword(base::ASCIIToUTF16("test.com"));
  result.SetURL("http://test.com/search?t={searchTerms}");
  return result;
}

IN_PROC_BROWSER_TEST_F(SearchEngineProviderControllerTest, PrefTest) {
  Profile* profile = browser()->profile();
  Profile* incognito_profile = profile->GetOffTheRecordProfile();

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  auto* incognito_service =
      TemplateURLServiceFactory::GetForProfile(incognito_profile);

  // Test pref is initially disabled.
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(profile));

  // Both mode should use same search engine if alternate pref is disabled.
  base::string16 normal_search_engine =
      service->GetDefaultSearchProvider()->data().short_name();
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            incognito_service->GetDefaultSearchProvider()->data().short_name());

  // Toggle pref and check incognito_service uses duckduckgo search engine and
  // normal mode service uses existing one.
  brave::ToggleUseAlternativeSearchEngineProvider(profile);
  EXPECT_TRUE(brave::UseAlternativeSearchEngineProviderEnabled(profile));
  EXPECT_EQ(incognito_service->GetDefaultSearchProvider()->data().short_name(),
            base::ASCIIToUTF16("DuckDuckGo"));
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            normal_search_engine);

  // Toggle pref again and check both mode uses same search engine.
  brave::ToggleUseAlternativeSearchEngineProvider(profile);
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(profile));
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            normal_search_engine);
  EXPECT_EQ(incognito_service->GetDefaultSearchProvider()->data().short_name(),
            normal_search_engine);

  // Check private search engine uses normal mode search engine when alternative
  // search engine pref is false.
  TemplateURLData test_data = CreateTestSearchEngine();
  std::unique_ptr<TemplateURL> test_url(new TemplateURL(test_data));
  service->SetUserSelectedDefaultSearchProvider(test_url.get());
  EXPECT_EQ(incognito_service->GetDefaultSearchProvider()->data().short_name(),
            base::ASCIIToUTF16("test1"));
}

// Check crash isn't happened with multiple private window is used.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderControllerTest,
                       MultiplePrivateWindowTest) {
  Browser* private_window_1 = CreateIncognitoBrowser();
  CloseBrowserSynchronously(private_window_1);

  Browser* private_window_2 = CreateIncognitoBrowser();
  brave::ToggleUseAlternativeSearchEngineProvider(private_window_2->profile());
}

// This test crashes with below. I don't know how to deal with now.
// [FATAL:brave_content_browser_client.cc(217)] Check failed: !path.empty().
// TODO(simonhong): Enable this later.
IN_PROC_BROWSER_TEST_F(TorSearchEngineProviderControllerTest,
                       DISABLED_CheckTorProfileSearchProviderTest) {
  base::FilePath tor_path = BraveProfileManager::GetTorProfilePath();
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  Profile* tor_profile = profile_manager->GetProfile(tor_path);
  EXPECT_TRUE(tor_profile->IsTorProfile());

  auto* service = TemplateURLServiceFactory::GetForProfile(tor_profile);
  //Check tor profile's search provider is set to ddg.
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            base::ASCIIToUTF16("DuckDuckGo"));

  content::RunAllTasksUntilIdle();
}
