/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier.h"

#include "base/containers/fixed_flat_set.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "url/gurl.h"

namespace serp_metrics {

namespace {

constexpr auto kAllowList = base::MakeFixedFlatSet<SearchEngineType>(
    base::sorted_unique,
    {SEARCH_ENGINE_BING, SEARCH_ENGINE_GOOGLE, SEARCH_ENGINE_YAHOO,
     SEARCH_ENGINE_DUCKDUCKGO, SEARCH_ENGINE_QWANT, SEARCH_ENGINE_ECOSIA,
     SEARCH_ENGINE_BRAVE, SEARCH_ENGINE_STARTPAGE});

// Returns a `TemplateURL` if `url` matches the search engine results page for
// `prepopulated_engine`.
std::unique_ptr<TemplateURL> MaybeGetTemplateURLForPrepopulatedEngine(
    const TemplateURLPrepopulateData::PrepopulatedEngine& prepopulated_engine,
    const GURL& url) {
  if (!kAllowList.contains(prepopulated_engine.type)) {
    return nullptr;
  }

  const auto template_url_data =
      TemplateURLDataFromPrepopulatedEngine(prepopulated_engine);
  auto template_url = std::make_unique<TemplateURL>(*template_url_data);

  if (template_url->type() != TemplateURL::NORMAL) {
    // Ignore non-standard search engines (e.g. extension/omnibox).
    return nullptr;
  }

  if (!template_url->IsSearchURL(url, SearchTermsData())) {
    return nullptr;
  }

  return template_url;
}

}  // namespace

bool SerpClassifier::IsSameSearchQuery(const GURL& lhs, const GURL& rhs) const {
  return NormalizeUrl(lhs) == NormalizeUrl(rhs);
}

std::optional<SearchEngineType> SerpClassifier::MaybeClassify(const GURL& url) {
  const GURL normalized_url = NormalizeUrl(url);

  if (const auto template_url = MaybeGetTemplateUrl(normalized_url)) {
    return template_url->GetEngineType(SearchTermsData());
  }

  return std::nullopt;
}

///////////////////////////////////////////////////////////////////////////////

GURL SerpClassifier::NormalizeUrl(const GURL& url) const {
  if (!url.is_valid()) {
    return url;
  }

  // Strip the port. Search engine template search URLs never include explicit
  // ports, and test servers use random ones. Google tests handle this via
  // `switches::kIgnoreGooglePortNumbers`, but that switch is Google-specific
  // and does not apply to other hosts.
  GURL::Replacements url_replacements;
  url_replacements.ClearPort();
  GURL normalized_url = url.ReplaceComponents(url_replacements);

  if (const auto template_url = MaybeGetTemplateUrl(normalized_url)) {
    template_url->KeepSearchTermsInURL(normalized_url, SearchTermsData(),
                                       /*keep_search_intent_params=*/false,
                                       /*normalize_search_terms=*/true,
                                       /*out_url=*/&normalized_url,
                                       /*out_search_terms=*/nullptr);
  }

  return normalized_url;
}

std::unique_ptr<TemplateURL> SerpClassifier::MaybeGetTemplateUrl(
    const GURL& url) const {
  for (const auto* prepopulated_engine :
       TemplateURLPrepopulateData::GetAllPrepopulatedEngines()) {
    if (auto search_engine = MaybeGetTemplateURLForPrepopulatedEngine(
            *prepopulated_engine, url)) {
      return search_engine;
    }
  }

  for (const auto& [_, prepopulated_engine] :
       TemplateURLPrepopulateData::kBraveEngines) {
    if (auto search_engine = MaybeGetTemplateURLForPrepopulatedEngine(
            *prepopulated_engine, url)) {
      return search_engine;
    }
  }

  return nullptr;
}

}  // namespace serp_metrics
