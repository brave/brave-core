/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier.h"

#include <memory>
#include <string_view>

#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/serp_metrics/serp_classifier_utils.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "url/gurl.h"

namespace serp_metrics {

namespace {

constexpr std::string_view kStartpageUrlHost = "www.startpage.com";
constexpr std::string_view kStartpageUrlPath = "/sp/search";

// Returns a `TemplateURL` if `url` matches the search engine results page for
// `prepopulated_engine`.
std::unique_ptr<TemplateURL> MaybeGetTemplateURLForPrepopulatedEngine(
    const TemplateURLPrepopulateData::PrepopulatedEngine& prepopulated_engine,
    const GURL& url) {
  if (!IsAllowedSearchEngine(prepopulated_engine.type)) {
    return nullptr;
  }

  const auto template_url_data =
      TemplateURLDataFromPrepopulatedEngine(prepopulated_engine);
  auto template_url = std::make_unique<TemplateURL>(*template_url_data);

  if (!template_url->IsSearchURL(url, SearchTermsData())) {
    if (prepopulated_engine.type == SEARCH_ENGINE_STARTPAGE &&
        url.host() == kStartpageUrlHost && url.path() == kStartpageUrlPath) {
      // Startpage uses a path-based SERP URL. Chromium still checks the legacy
      // query-based format and does not support the new one. Even if we update
      // the search URL, `TemplateURL::IsSearchURL` still fails because it
      // requires non-empty search terms.
      return template_url;
    }

    return nullptr;
  }

  return template_url;
}

// Returns a `TemplateURL` if `url` matches the search engine results page for
// any prepopulated engine in the allow list.
std::unique_ptr<TemplateURL> MaybeGetTemplateUrl(const GURL& url) {
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

// Normalizes a SERP URL so equivalent search queries produce the same URL for
// comparison. Strips ports, removes non-search parameters, and canonicalizes
// the search terms.
GURL NormalizeUrl(const GURL& url) {
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

}  // namespace

bool IsSameSearchQuery(const GURL& lhs, const GURL& rhs) {
  if (lhs.host() == kStartpageUrlHost) {
    // For Startpage, we cannot determine whether two URLs represent the same
    // search results page, so these pages are always classified.
    return false;
  }

  return NormalizeUrl(lhs) == NormalizeUrl(rhs);
}

std::optional<SearchEngineType> MaybeClassifySearchEngine(const GURL& url) {
  const GURL normalized_url = NormalizeUrl(url);
  if (const auto template_url = MaybeGetTemplateUrl(normalized_url)) {
    return template_url->GetEngineType(SearchTermsData());
  }

  return std::nullopt;
}

}  // namespace serp_metrics
