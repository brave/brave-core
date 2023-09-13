/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/profile_resetter/brave_profile_resetter.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/pref_names.h"
#include "brave/browser/search_engines/search_engine_provider_service_factory.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/profile_resetter/brandcoded_default_settings.h"
#include "chrome/browser/profile_resetter/profile_resetter_test_base.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/country_codes/country_codes.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/search_engines_test_util.h"
#include "components/search_engines/template_url_data_util.h"
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

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

using SearchEngineProviderServiceTest = InProcessBrowserTest;

namespace {

testing::AssertionResult VerifyTemplateURLServiceLoad(
    TemplateURLService* service) {
  if (service->loaded())
    return testing::AssertionSuccess();
  search_test_utils::WaitForTemplateURLServiceToLoad(service);
  if (service->loaded())
    return testing::AssertionSuccess();
  return testing::AssertionFailure() << "TemplateURLService isn't loaded";
}

TemplateURLData CreateTestSearchEngine() {
  TemplateURLData result;
  result.SetShortName(u"test1");
  result.SetKeyword(u"test.com");
  result.SetURL("http://test.com/search?t={searchTerms}");
  return result;
}

std::string GetBraveSearchProviderSyncGUID(PrefService* prefs) {
  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      prefs, TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE);
  DCHECK(data);
  return data->sync_guid;
}

bool PrepopulatedDataHasDDG(PrefService* prefs) {
  static constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
      alt_search_providers[] = {
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
          TemplateURLPrepopulateData::
              PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE};

  for (const auto& id : alt_search_providers) {
    if (TemplateURLPrepopulateData::GetPrepopulatedEngine(prefs, id)) {
      return true;
    }
  }

  return false;
}

}  // namespace

// Set alternative search provider prefs and check it's cleared on next
// launching.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       PRE_PrivateSearchProviderMigrationTest) {
  auto* prefs = browser()->profile()->GetPrefs();
  // Set "US" to make prepopluated data include DDG because this migration test
  // is for checking alternative search provider(DDG) is set to private search
  // provider properly.
  prefs->SetInteger(country_codes::kCountryIDAtInstall, 'U' << 8 | 'S');

  ASSERT_TRUE(PrepopulatedDataHasDDG(prefs));
  prefs->SetBoolean(kShowAlternativePrivateSearchEngineProviderToggle, true);
  prefs->SetBoolean(kUseAlternativePrivateSearchEngineProvider, true);
}

IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       PrivateSearchProviderMigrationTest) {
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetInteger(country_codes::kCountryIDAtInstall, 'U' << 8 | 'S');
  ASSERT_TRUE(PrepopulatedDataHasDDG(prefs));

  EXPECT_FALSE(
      prefs->GetBoolean(kShowAlternativePrivateSearchEngineProviderToggle));
  EXPECT_FALSE(prefs->GetBoolean(kUseAlternativePrivateSearchEngineProvider));
}

IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       PRE_InvalidPrivateSearchProviderRestoreTest) {
  auto* profile = browser()->profile();
  profile->GetPrefs()->SetString(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                                 "invalid_id");
}

IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       InvalidPrivateSearchProviderRestoreTest) {
  auto* profile = browser()->profile();
  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  EXPECT_TRUE(VerifyTemplateURLServiceLoad(service));

  EXPECT_EQ(GetBraveSearchProviderSyncGUID(profile->GetPrefs()),
            profile->GetPrefs()->GetString(
                prefs::kSyncedDefaultPrivateSearchProviderGUID));
}

// Check crash isn't happened with multiple private window is used.
// https://github.com/brave/brave-browser/issues/1452
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       MultiplePrivateWindowTest) {
  Browser* private_window_1 = CreateIncognitoBrowser();
  CloseBrowserSynchronously(private_window_1);

  Browser* private_window_2 = CreateIncognitoBrowser();
  CloseBrowserSynchronously(private_window_2);
}

// Check default search provider in private/tor window.
IN_PROC_BROWSER_TEST_F(SearchEngineProviderServiceTest,
                       CheckDefaultSearchProviderTest) {
  Profile* profile = browser()->profile();
  Profile* incognito_profile =
      profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);

  // Need to wait as
  // PrivateWindowSearchEngineProviderServiceBase::Initialize() is
  // run as posted task.
  base::RunLoop().RunUntilIdle();

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  EXPECT_TRUE(VerifyTemplateURLServiceLoad(service));

  // Check TemplateURLData for private window search provider is set
  // properly.
  auto* preference = profile->GetPrefs()->FindPreference(
      prefs::kSyncedDefaultPrivateSearchProviderData);
  EXPECT_FALSE(preference->IsDefaultValue());
  // Cache initial private search provider sync guid to compare later
  // after changing search provider.
  const auto initial_private_url_data_sync_guid =
      TemplateURLDataFromDictionary(preference->GetValue()->GetDict())
          ->sync_guid;
  auto* incognito_service =
      TemplateURLServiceFactory::GetForProfile(incognito_profile);
  const int initial_normal_provider_id =
      service->GetDefaultSearchProvider()->prepopulate_id();
  const int initial_private_provider_id =
      incognito_service->GetDefaultSearchProvider()->prepopulate_id();
  // Check Brave Search is default provider for private window.
  EXPECT_EQ(static_cast<int>(
                TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE),
            initial_private_provider_id);

  // Check changing normal provider doesn't affect private provider.
  TemplateURLData test_data = CreateTestSearchEngine();
  std::unique_ptr<TemplateURL> test_url(new TemplateURL(test_data));
  service->SetUserSelectedDefaultSearchProvider(test_url.get());
  EXPECT_NE(initial_normal_provider_id,
            service->GetDefaultSearchProvider()->prepopulate_id());
  EXPECT_EQ(initial_private_provider_id,
            incognito_service->GetDefaultSearchProvider()->prepopulate_id());

  // Change private search provider.
  const auto new_private_url_data_sync_guid = test_url->sync_guid();
  service->Add(std::move(test_url));
  profile->GetPrefs()->SetString(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                                 new_private_url_data_sync_guid);

  // Check url data is updated properly after chaning private search provider.
  EXPECT_EQ(new_private_url_data_sync_guid,
            TemplateURLDataFromDictionary(preference->GetValue()->GetDict())
                ->sync_guid);
  EXPECT_NE(initial_private_provider_id,
            incognito_service->GetDefaultSearchProvider()->prepopulate_id());

  // Reset and check initial one is set.
  BraveProfileResetter resetter(profile);
  std::unique_ptr<BrandcodedDefaultSettings> master_settings(
      new BrandcodedDefaultSettings);
  ProfileResetterMockObject mock_object;
  resetter.Reset(ProfileResetter::DEFAULT_SEARCH_ENGINE,
                 std::move(master_settings),
                 base::BindOnce(&ProfileResetterMockObject::StopLoop,
                                base::Unretained(&mock_object)));
  mock_object.RunLoop();
  EXPECT_EQ(initial_private_provider_id,
            incognito_service->GetDefaultSearchProvider()->prepopulate_id());

  // Set invalid private search provider id and check default provider is set
  // properly.
  profile->GetPrefs()->SetString(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                                 "invalid_id");
  EXPECT_EQ(GetBraveSearchProviderSyncGUID(profile->GetPrefs()),
            profile->GetPrefs()->GetString(
                prefs::kSyncedDefaultPrivateSearchProviderGUID));
  EXPECT_EQ(initial_private_provider_id,
            incognito_service->GetDefaultSearchProvider()->prepopulate_id());

#if BUILDFLAG(ENABLE_TOR)
  Browser* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->profile());
  Profile* tor_profile = tor_browser->profile();
  EXPECT_TRUE(tor_profile->IsTor());

  // Wait for the search provider to initialize.
  base::RunLoop().RunUntilIdle();

  const int default_provider_id =
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE_TOR;
  auto* tor_service = TemplateURLServiceFactory::GetForProfile(tor_profile);
  EXPECT_EQ(tor_service->GetDefaultSearchProvider()->prepopulate_id(),
            default_provider_id);
#endif
}

#if BUILDFLAG(ENABLE_EXTENSIONS)

namespace extensions {

// Copied from settings_overrides_browsertest.cc
// On linux, search engine from extension is not set by default.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
// Prepopulated id hardcoded in test_extension.
const int kTestExtensionPrepopulatedId = 3;
// TemplateURLData with search engines settings from test extension manifest.
// chrome/test/data/extensions/settings_override/manifest.json
std::unique_ptr<TemplateURLData> TestExtensionSearchEngine(PrefService* prefs) {
  auto result = std::make_unique<TemplateURLData>();
  result->SetShortName(u"name.de");
  result->SetKeyword(u"keyword.de");
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

  // Need to wait as
  // PrivateWindowSearchEngineProviderServiceBase::Initialize() is
  // run as posted task.
  base::RunLoop().RunUntilIdle();

  auto* incognito_url_service =
      TemplateURLServiceFactory::GetForProfile(incognito_profile);
  const TemplateURL* current_incognito_dse =
      incognito_url_service->GetDefaultSearchProvider();
  EXPECT_EQ(TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION,
            current_incognito_dse->type());

  // Check that extension's provider is still used when private window's search
  // provider option is changed.
  TemplateURLData test_data = CreateTestSearchEngine();
  std::unique_ptr<TemplateURL> test_url(new TemplateURL(test_data));
  incognito_url_service->SetUserSelectedDefaultSearchProvider(test_url.get());

  current_incognito_dse = incognito_url_service->GetDefaultSearchProvider();
  EXPECT_EQ(TemplateURL::NORMAL_CONTROLLED_BY_EXTENSION,
            current_incognito_dse->type());

  UnloadExtension(extension->id());
  EXPECT_EQ(default_provider, url_service->GetDefaultSearchProvider());

  // Check Brave Search is back to as a default provider for private window
  // after unloading extension.
  current_incognito_dse = incognito_url_service->GetDefaultSearchProvider();
  EXPECT_EQ(static_cast<int>(
                TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE),
            current_incognito_dse->prepopulate_id());
  EXPECT_EQ(TemplateURL::NORMAL, current_incognito_dse->type());
}
#endif

}  // namespace extensions

#endif  // ENABLE_EXTENSIONS
