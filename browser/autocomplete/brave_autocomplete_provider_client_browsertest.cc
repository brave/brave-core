/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_provider_client.h"

#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"

using BraveAutocompleteProviderClientTest = InProcessBrowserTest;

// BraveAutocompleteProviderClient should only use different TemplateURLService.
// And Other service should be same for normal/incognito profile.
IN_PROC_BROWSER_TEST_F(BraveAutocompleteProviderClientTest,
                       DependentServiceCheckTest) {
  Profile* profile = browser()->profile();
  Profile* incognito_profile = profile->GetOffTheRecordProfile();

  // Brave initiates different AutocompleteClassifier service for
  // normal/incognito profile.
  EXPECT_NE(AutocompleteClassifierFactory::GetForProfile(profile),
            AutocompleteClassifierFactory::GetForProfile(incognito_profile));

  BraveAutocompleteProviderClient normal_client(profile);
  BraveAutocompleteProviderClient incognito_client(incognito_profile);

  // Check different TemplateURLService is used.
  EXPECT_NE(normal_client.GetTemplateURLService(),
            incognito_client.GetTemplateURLService());

  // Check same services are used except TemplateURLService.
  EXPECT_EQ(normal_client.GetAutocompleteClassifier(),
            incognito_client.GetAutocompleteClassifier());
  EXPECT_EQ(normal_client.GetHistoryService(),
            incognito_client.GetHistoryService());
  EXPECT_EQ(normal_client.GetRemoteSuggestionsService(true),
            incognito_client.GetRemoteSuggestionsService(true));
  EXPECT_EQ(normal_client.GetDocumentSuggestionsService(true),
            incognito_client.GetDocumentSuggestionsService(true));
}
