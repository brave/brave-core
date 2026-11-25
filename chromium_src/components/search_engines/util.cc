/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/util.h"

#include <algorithm>
#include <ranges>

#define GetSearchProvidersUsingKeywordResult \
  GetSearchProvidersUsingKeywordResult_ChromiumImpl
#include <components/search_engines/util.cc>
#undef GetSearchProvidersUsingKeywordResult

void GetSearchProvidersUsingKeywordResult(
    const WDKeywordsResult& keyword_result,
    KeywordWebDataService* service,
    PrefService* prefs,
    const TemplateURLPrepopulateData::Resolver& template_url_data_resolver,
    TemplateURLService::OwnedTemplateURLVector* template_urls,
    TemplateURL* default_search_provider,
    const SearchTermsData& search_terms_data,
    WDKeywordsResult::Metadata& out_updated_keywords_metadata,
    std::set<std::string>* removed_keyword_guids) {
  // Call the original implementation to get template_urls.
  GetSearchProvidersUsingKeywordResult_ChromiumImpl(
      keyword_result, service, prefs, template_url_data_resolver, template_urls,
      default_search_provider, search_terms_data, out_updated_keywords_metadata,
      removed_keyword_guids);
  // Resort template_urls in the order of prepopulated search engines.
  if (template_urls && !template_urls->empty()) {
    TemplateURLPrepopulateData::PrepopulatedEngineVector prepopulated_engines =
        template_url_data_resolver.GetPrepopulatedEngines();
    for (const auto& engine : std::views::reverse(prepopulated_engines)) {
      auto it = std::ranges::find_if(
          *template_urls, [&engine](std::unique_ptr<TemplateURL>& t_url1) {
            return t_url1->prepopulate_id() == engine->id;
          });
      if (it != end(*template_urls))
        std::rotate(begin(*template_urls), it, it + 1);
    }
  }
}
