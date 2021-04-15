/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/guest_window_search_engine_provider_service.h"
#include "brave/browser/search_engines/search_engine_provider_service_factory.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/search_engines/search_engines_test_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_browsertest.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/extension.h"
#endif

using SearchEngineProviderServiceTest = InProcessBrowserTest;

TemplateURLData CreateTestSearchEngine() {
  TemplateURLData result;
  result.SetShortName(u"test1");
  result.SetKeyword(u"test.com");
  result.SetURL("http://test.com/search?t={searchTerms}");
  return result;
}

// In Qwant region, alternative search engine prefs isn't used.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       PrivateWindowPrefTestWithNonQwantRegion) {
  Profile* profile = browser()->profile();
  Profile* incognito_profile =
      profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);

  // This test case is only for non-qwant region.
  if (brave::IsRegionForQwant(profile))
    return;

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  auto* incognito_service =
      TemplateURLServiceFactory::GetForProfile(incognito_profile);

  // Test pref is initially disabled.
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(profile));

  // Both mode should use same search engine if alternate pref is disabled.
  std::u16string normal_search_engine =
      service->GetDefaultSearchProvider()->data().short_name();
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            incognito_service->GetDefaultSearchProvider()->data().short_name());

  // Toggle pref and check incognito_service uses duckduckgo search engine and
  // normal mode service uses existing one.
  brave::ToggleUseAlternativeSearchEngineProvider(profile);
  EXPECT_TRUE(brave::UseAlternativeSearchEngineProviderEnabled(profile));
  EXPECT_EQ(incognito_service->GetDefaultSearchProvider()->data().short_name(),
            u"DuckDuckGo");
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
            u"test1");
}

// For qwant region, just check that both profile uses same provider.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       PrivateWindowTestWithQwantRegion) {
  Profile* profile = browser()->profile();
  Profile* incognito_profile =
      profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);

  // This test case is only for qwant region.
  if (!brave::IsRegionForQwant(profile))
    return;

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  auto* incognito_service =
      TemplateURLServiceFactory::GetForProfile(incognito_profile);

  // Test pref is initially disabled.
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(profile));

  // Toggling doesn't work in qwant region.
  brave::ToggleUseAlternativeSearchEngineProvider(profile);
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(profile));

  // Both mode should use same search engine.
  std::u16string normal_search_engine =
      service->GetDefaultSearchProvider()->data().short_name();
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            incognito_service->GetDefaultSearchProvider()->data().short_name());


  // Check private search engine uses normal mode search engine.
  TemplateURLData test_data = CreateTestSearchEngine();
  std::unique_ptr<TemplateURL> test_url(new TemplateURL(test_data));
  service->SetUserSelectedDefaultSearchProvider(test_url.get());
  EXPECT_EQ(incognito_service->GetDefaultSearchProvider()->data().short_name(),
            u"test1");
}

// Check crash isn't happened with multiple private window is used.
// https://github.com/brave/brave-browser/issues/1452
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       MultiplePrivateWindowTest) {
  Browser* private_window_1 = CreateIncognitoBrowser();
  CloseBrowserSynchronously(private_window_1);

  Browser* private_window_2 = CreateIncognitoBrowser();
  brave::ToggleUseAlternativeSearchEngineProvider(private_window_2->profile());
}

#if BUILDFLAG(ENABLE_TOR)
// Checks the default search engine of the tor profile.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       PRE_CheckDefaultTorProfileSearchProviderTest) {
  brave::NewOffTheRecordWindowTor(browser());
  content::RunAllTasksUntilIdle();

  Profile* tor_profile = BrowserList::GetInstance()->GetLastActive()->profile();
  EXPECT_TRUE(tor_profile->IsTor());

  auto* service = TemplateURLServiceFactory::GetForProfile(tor_profile);

  int default_provider_id = brave::IsRegionForQwant(tor_profile) ?
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT :
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO;

  // Check tor profile's search provider is set to ddg.
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().prepopulate_id,
            default_provider_id);

  // Change provider to check whether it is retained in the next sessions.
  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      tor_profile->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING);
  TemplateURL other_url(*data);
  service->SetUserSelectedDefaultSearchProvider(&other_url);
}

// Check changed provider in tor profile should not be retained across the
// sessions.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       CheckDefaultTorProfileSearchProviderTest) {
  brave::NewOffTheRecordWindowTor(browser());
  content::RunAllTasksUntilIdle();

  Profile* tor_profile = BrowserList::GetInstance()->GetLastActive()->profile();
  EXPECT_TRUE(tor_profile->IsTor());

  int default_provider_id =
      brave::IsRegionForQwant(tor_profile)
          ? TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT
          : TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO;
  auto* service = TemplateURLServiceFactory::GetForProfile(tor_profile);
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().prepopulate_id,
            default_provider_id);
}
#endif

// Check ddg toggle button state is changed by user's settings change.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       GuestWindowControllerTest) {
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

  auto* search_engine_provider_service =
      static_cast<GuestWindowSearchEngineProviderService*>(
          SearchEngineProviderServiceFactory::GetForProfile(guest_profile));
  search_engine_provider_service->OnTemplateURLServiceChanged();

  // Check alternative pref is turned off.
  EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(guest_profile));

  auto ddg_data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      guest_profile->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO);
  TemplateURL ddg_url(*ddg_data);

  template_service->SetUserSelectedDefaultSearchProvider(&ddg_url);
  search_engine_provider_service->OnTemplateURLServiceChanged();
  EXPECT_TRUE(brave::UseAlternativeSearchEngineProviderEnabled(guest_profile));
}

#if BUILDFLAG(ENABLE_EXTENSIONS)

namespace extensions {

// Copied from settings_overrides_browsertest.cc
// On linux, search engine from extension is not set by default.
#if defined(OS_WIN) || defined(OS_MAC)
// Prepopulated id hardcoded in test_extension.
const int kTestExtensionPrepopulatedId = 3;
// TemplateURLData with search engines settings from test extension manifest.
// chrome/test/data/extensions/settings_override/manifest.json
std::unique_ptr<TemplateURLData> TestExtensionSearchEngine(PrefService* prefs) {
  auto result = std::make_unique<TemplateURLData>();
  result->SetShortName(base::ASCIIToUTF16("name.de"));
  result->SetKeyword(base::ASCIIToUTF16("keyword.de"));
  result->SetURL("http://www.foo.de/s?q={searchTerms}&id=10");
  result->favicon_url = GURL("http://www.foo.de/favicon.ico?id=10");
  result->suggestions_url = "http://www.foo.de/suggest?q={searchTerms}&id=10";
  result->image_url = "http://www.foo.de/image?q={searchTerms}&id=10";
  result->search_url_post_params = "search_lang=de";
  result->suggestions_url_post_params = "suggest_lang=de";
  result->image_url_post_params = "image_lang=de";
  result->alternate_urls.push_back("http://www.moo.de/s?q={searchTerms}&id=10");
  result->alternate_urls.push_back("http://www.noo.de/s?q={searchTerms}&id=10");
  result->input_encodings.push_back("UTF-8");

  std::unique_ptr<TemplateURLData> prepopulated =
      TemplateURLPrepopulateData::GetPrepopulatedEngine(
          prefs, kTestExtensionPrepopulatedId);
  // Values below do not exist in extension manifest and are taken from
  // prepopulated engine with prepopulated_id set in extension manifest.
  result->contextual_search_url = prepopulated->contextual_search_url;
  result->new_tab_url = prepopulated->new_tab_url;
  return result;
}

testing::AssertionResult VerifyTemplateURLServiceLoad(
    TemplateURLService* service) {
  if (service->loaded())
    return testing::AssertionSuccess();
  search_test_utils::WaitForTemplateURLServiceToLoad(service);
  if (service->loaded())
    return testing::AssertionSuccess();
  return testing::AssertionFailure() << "TemplateURLService isn't loaded";
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest,
                       ExtensionSearchProviderWithPrivateWindow) {
  PrefService* prefs = profile()->GetPrefs();
  ASSERT_TRUE(prefs);
  TemplateURLService* url_service =
      TemplateURLServiceFactory::GetForProfile(profile());
  ASSERT_TRUE(url_service);
  EXPECT_TRUE(VerifyTemplateURLServiceLoad(url_service));
  const TemplateURL* default_provider = url_service->GetDefaultSearchProvider();
  ASSERT_TRUE(default_provider);
  EXPECT_EQ(TemplateURL::NORMAL, default_provider->type());

  const extensions::Extension* extension = LoadExtension(
      test_data_dir_.AppendASCII("settings_override"), {.install_param = "10"});
  ASSERT_TRUE(extension);
  const TemplateURL* current_dse = url_service->GetDefaultSearchProvider();
  EXPECT_EQ(TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION, current_dse->type());

  std::unique_ptr<TemplateURLData> extension_dse =
      TestExtensionSearchEngine(prefs);
  ExpectSimilar(extension_dse.get(), &current_dse->data());

  Profile* incognito_profile =
      profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true);

  auto* incognito_url_service =
      TemplateURLServiceFactory::GetForProfile(incognito_profile);
  const TemplateURL* current_incognito_dse =
      incognito_url_service->GetDefaultSearchProvider();
  EXPECT_EQ(TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION,
            current_incognito_dse->type());

  if (!brave::IsRegionForQwant(profile())) {
    // DDG toggle button is on and its preference is stored.
    // but search provider is not changed. still extension search provider is
    // used.
    EXPECT_FALSE(brave::UseAlternativeSearchEngineProviderEnabled(profile()));
    brave::ToggleUseAlternativeSearchEngineProvider(profile());
    EXPECT_TRUE(brave::UseAlternativeSearchEngineProviderEnabled(profile()));
    current_incognito_dse = incognito_url_service->GetDefaultSearchProvider();
    EXPECT_EQ(TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION,
              current_incognito_dse->type());
  }

  UnloadExtension(extension->id());
  EXPECT_EQ(default_provider, url_service->GetDefaultSearchProvider());

  // After unloading private window's search provider is ddg.
  current_incognito_dse = incognito_url_service->GetDefaultSearchProvider();
  EXPECT_EQ(current_incognito_dse->data().short_name(),
            base::ASCIIToUTF16("DuckDuckGo"));
  EXPECT_EQ(TemplateURL::NORMAL, current_incognito_dse->type());
}
#endif

}  // namespace extensions

#endif  // ENABLE_EXTENSIONS
