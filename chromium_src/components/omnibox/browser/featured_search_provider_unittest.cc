/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url_starter_pack_data.h"

// Let the upstream unit test use upstream's list of starter pack engines,
// rather than Brave's overridden list.
#define GetStarterPackEngines GetStarterPackEngines_ChromiumImpl

#include <components/omnibox/browser/featured_search_provider_unittest.cc>

#undef GetStarterPackEngines

namespace {

constexpr char16_t kAskBraveSearchKeyword[] = u"@ask";

const auto* kAskBraveSearchUrl =
    template_url_starter_pack_data::ask_brave_search.destination_url;

}  // namespace

TEST_F(FeaturedSearchProviderTest, BraveStarterPacks) {
  std::vector<std::unique_ptr<TemplateURLData>> turls =
      template_url_starter_pack_data::GetStarterPackEngines();
  for (auto& turl : turls) {
    client_->GetTemplateURLService()->Add(
        std::make_unique<TemplateURL>(std::move(*turl)));
  }

  std::vector<TestData> typing_scheme_cases = {
      // Typing the keyword without '@' or past the keyword shouldn't produce
      // results.
      {u"ask", {}},
      {u"@askk", {}},

      // Typing '@' should give all the starter pack suggestions.
      {u"@", {kAskBraveSearchUrl, kBookmarksUrl, kHistoryUrl, kTabsUrl}},

      // Typing a portion of "@ask" should give the Ask Brave Search suggestion.
      {std::u16string(kAskBraveSearchKeyword, 0, 2), {kAskBraveSearchUrl}},
      {kAskBraveSearchKeyword, {kAskBraveSearchUrl}},
  };

  RunTest(typing_scheme_cases);
}
