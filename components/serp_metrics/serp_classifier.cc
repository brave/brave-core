/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier.h"

#include <memory>
#include <string_view>

#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/serp_metrics/serp_classifier_utils.h"
#include "components/regional_capabilities/regional_capabilities_utils.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_starter_pack_data.h"
#include "url/gurl.h"

namespace serp_metrics {

namespace {

constexpr std::string_view kStartpageUrlHost = "www.startpage.com";
constexpr std::string_view kStartpageUrlPath = "/sp/search";

// Returns a `TemplateURL` built from `template_url_data` if `url` matches its
// search URL. If `allow_startpage_fallback` is true, also returns the built
// `TemplateURL` when `url` matches Startpage's path-based SERP URL.
std::unique_ptr<TemplateURL> MaybeGetTemplateURLIfSearchUrl(
    std::unique_ptr<TemplateURLData> template_url_data,
    const GURL& url,
    bool allow_startpage_fallback) {
  auto template_url = std::make_unique<TemplateURL>(*template_url_data);

  if (template_url->IsSearchURL(url, SearchTermsData())) {
    return template_url;
  }

  if (allow_startpage_fallback && url.host() == kStartpageUrlHost &&
      url.path() == kStartpageUrlPath) {
    // Startpage uses a path-based SERP URL. Chromium still checks the legacy
    // query-based format and does not support the new one. Even if we update
    // the search URL, `TemplateURL::IsSearchURL` still fails because it
    // requires non-empty search terms.
    return template_url;
  }

  return nullptr;
}

// Returns a `TemplateURL` if `url` matches the search engine results page for
// `prepopulated_engine`.
std::unique_ptr<TemplateURL> MaybeGetTemplateURLForPrepopulatedEngine(
    const TemplateURLPrepopulateData::PrepopulatedEngine& prepopulated_engine,
    const GURL& url) {
  if (!IsAllowedSearchEngine(prepopulated_engine.type)) {
    return nullptr;
  }

  return MaybeGetTemplateURLIfSearchUrl(
      TemplateURLDataFromPrepopulatedEngine(prepopulated_engine), url,
      /*allow_startpage_fallback=*/prepopulated_engine.type ==
          SEARCH_ENGINE_STARTPAGE);
}

// Returns a `TemplateURL` if `url` matches the Ask Brave Search starter pack.
// Unlike `MaybeGetTemplateURLForPrepopulatedEngine`, this is not gated by
// `IsAllowedSearchEngine` because `SEARCH_ENGINE_STARTER_PACK_ASK_BRAVE_SEARCH`
// is not a prepopulated engine type and is always classified as Brave Search.
std::unique_ptr<TemplateURL> MaybeGetTemplateURLForAskBraveSearch(
    const GURL& url) {
  return MaybeGetTemplateURLIfSearchUrl(
      TemplateURLDataFromStarterPackEngine(
          template_url_starter_pack_data::ask_brave_search),
      url, /*allow_startpage_fallback=*/false);
}

// Returns a `TemplateURL` if `url` matches the search engine results page for
// any prepopulated engine in the allow list, or the Ask Brave Search starter
// pack.
std::unique_ptr<TemplateURL> MaybeGetTemplateUrl(const GURL& url) {
  for (const TemplateURLPrepopulateData::PrepopulatedEngine*
           prepopulated_engine :
       regional_capabilities::GetAllPrepopulatedEngines()) {
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

  if (auto search_engine = MaybeGetTemplateURLForAskBraveSearch(url)) {
    return search_engine;
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
  if (lhs.host() == kStartpageUrlHost || rhs.host() == kStartpageUrlHost) {
    // For Startpage, we cannot determine whether two URLs represent the same
    // search results page, so these pages are always classified.
    return false;
  }

  return NormalizeUrl(lhs) == NormalizeUrl(rhs);
}

std::optional<SearchEngineType> MaybeClassifySearchEngine(const GURL& url) {
  const GURL normalized_url = NormalizeUrl(url);
  if (const auto template_url = MaybeGetTemplateUrl(normalized_url)) {
    if (template_url->starter_pack_id() ==
        template_url_starter_pack_data::StarterPackId::kAskBraveSearch) {
      // Ask Brave Search is a starter pack, not a prepopulated engine, so
      // `GetEngineType()` cannot resolve it. It is still genuine use of Brave
      // Search, so it counts toward the same total.
      return SEARCH_ENGINE_BRAVE;
    }

    return template_url->GetEngineType(SearchTermsData());
  }

  return std::nullopt;
}

}  // namespace serp_metrics
