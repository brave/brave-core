/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_PROVIDER_CLIENT_H_
#define BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_PROVIDER_CLIENT_H_

#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"

// In brave, different AutocompleteClassifiers are created for normal and
// incognito profile by changing
// AutocompleteClassifierFactory::GetBrowserContextToUse().
// This changes are needed to use different search engine used by web search in
// web page context menu.
// When context menu handles web search it gets search engine url from
// ChromeAutocompleteProviderClient via AutocompleteClassifiers.
// Because of this, private window will use same search engine url of normal
// window if same AutocompleteClassifiers are used on normal and incognito.
// So, we made this change.
// However, ChromeAutocompleteProviderClient exposes many other services based
// on profiles.
// We don't want to change other services. Only wanted to get proper
// TemplateURLService. To achieve this, BraveAutocompleteProviderClient is
// introduced. It initializes ChromeAutocompleteProviderClient with original
// profile and only overrided TemplateURLService getter.
// BraveAutocompleteSchemeClassifier also initialize
// ChromeAutocompleteSchemeClassifier with original profile for same reason.
class BraveAutocompleteProviderClient
    : public ChromeAutocompleteProviderClient {
 public:
  explicit BraveAutocompleteProviderClient(Profile* profile);
  ~BraveAutocompleteProviderClient() override;

  TemplateURLService* GetTemplateURLService() override;
  const TemplateURLService* GetTemplateURLService() const override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BraveAutocompleteProviderClient);
};

#endif  // BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_PROVIDER_CLIENT_H_
