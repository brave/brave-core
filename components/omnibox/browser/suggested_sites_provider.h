/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_PROVIDER_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_PROVIDER_H_

#include <vector>
#include <string>

#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/omnibox/browser/suggested_sites_match.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider.h"

class AutocompleteProviderClient;

// This is the provider for Brave Suggested Sites
class SuggestedSitesProvider : public AutocompleteProvider {
 public:
  explicit SuggestedSitesProvider(AutocompleteProviderClient* client);
  SuggestedSitesProvider(const SuggestedSitesProvider&) = delete;
  SuggestedSitesProvider& operator=(const SuggestedSitesProvider&) = delete;

  // AutocompleteProvider:
  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~SuggestedSitesProvider() override;

  static const int kRelevance;

  const std::vector<SuggestedSitesMatch>& GetSuggestedSites();
  void AddMatch(const SuggestedSitesMatch& match,
                const ACMatchClassifications& styles);

  static ACMatchClassifications StylesForSingleMatch(
      const std::string &input_text,
      const std::string &site);

  raw_ptr<AutocompleteProviderClient> client_ = nullptr;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_PROVIDER_H_
