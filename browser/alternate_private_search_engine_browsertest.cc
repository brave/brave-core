/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/alternate_private_search_engine_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"

using AlternatePrivateSearchEngineTest = InProcessBrowserTest;

TemplateURLData CreateTestSearchEngine() {
  TemplateURLData result;
  result.SetShortName(base::ASCIIToUTF16("test1"));
  result.SetKeyword(base::ASCIIToUTF16("test.com"));
  result.SetURL("http://test.com/search?t={searchTerms}");
  return result;
}

IN_PROC_BROWSER_TEST_F(AlternatePrivateSearchEngineTest, PrefTest) {
  Profile* profile = browser()->profile();
  Profile* incognito_profile = profile->GetOffTheRecordProfile();

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  auto* incognito_service =
      TemplateURLServiceFactory::GetForProfile(incognito_profile);

  // Test pref is initially disabled.
  EXPECT_FALSE(brave::UseAlternatePrivateSearchEngineEnabled(profile));

  // Both mode should use same search engine if alternate pref is disabled.
  base::string16 normal_search_engine =
      service->GetDefaultSearchProvider()->data().short_name();
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            incognito_service->GetDefaultSearchProvider()->data().short_name());

  // Toggle pref and check incognito_service uses duckduckgo search engine and
  // normal mode service uses existing one.
  brave::ToggleUseAlternatePrivateSearchEngine(profile);
  EXPECT_TRUE(brave::UseAlternatePrivateSearchEngineEnabled(profile));
  EXPECT_EQ(incognito_service->GetDefaultSearchProvider()->data().short_name(),
            base::ASCIIToUTF16("DuckDuckGo"));
  EXPECT_EQ(service->GetDefaultSearchProvider()->data().short_name(),
            normal_search_engine);

  // Toggle pref again and check both mode uses same search engine.
  brave::ToggleUseAlternatePrivateSearchEngine(profile);
  EXPECT_FALSE(brave::UseAlternatePrivateSearchEngineEnabled(profile));
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
