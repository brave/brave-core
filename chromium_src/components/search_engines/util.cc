/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define GetSearchProvidersUsingKeywordResult \
  GetSearchProvidersUsingKeywordResult_ChromiumImpl
#include "../../../../components/search_engines/util.cc"
#undef GetSearchProvidersUsingKeywordResult

void GetSearchProvidersUsingKeywordResult(
    const WDTypedResult& result,
    KeywordWebDataService* service,
    PrefService* prefs,
    TemplateURLService::OwnedTemplateURLVector* template_urls,
    TemplateURL* default_search_provider,
    const SearchTermsData& search_terms_data,
    int* new_resource_keyword_version,
    std::set<std::string>* removed_keyword_guids) {
  // Call the original implementation to get template_urls.
  GetSearchProvidersUsingKeywordResult_ChromiumImpl(
      result, service, prefs, template_urls, default_search_provider,
      search_terms_data, new_resource_keyword_version, removed_keyword_guids);
  // Resort template_urls in the orider of prepopulated search engines.
  if (template_urls && !template_urls->empty()) {
    std::vector<std::unique_ptr<TemplateURLData>> prepopulated_urls =
        TemplateURLPrepopulateData::GetPrepopulatedEngines(prefs, nullptr);
    for (auto crit = prepopulated_urls.crbegin();
         crit != prepopulated_urls.crend(); ++crit) {
      auto it = find_if(
          begin(*template_urls), end(*template_urls),
          [&crit](std::unique_ptr<TemplateURL>& t_url1) {
            return (t_url1->prepopulate_id() == (*crit)->prepopulate_id);
          });
      if (it != end(*template_urls))
        std::rotate(begin(*template_urls), it, it + 1);
    }
  }
}
