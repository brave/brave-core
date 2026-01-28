/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier.h"

#include <string_view>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/feature_list.h"
#include "base/strings/pattern.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"

namespace metrics {

namespace {

constexpr auto kDisallowList = base::MakeFixedFlatSet<SearchEngineType>(
    base::sorted_unique,
    {
        SEARCH_ENGINE_UNKNOWN,
        SEARCH_ENGINE_STARTER_PACK_BOOKMARKS,
        SEARCH_ENGINE_STARTER_PACK_HISTORY,
        SEARCH_ENGINE_STARTER_PACK_TABS,
        SEARCH_ENGINE_STARTER_PACK_GEMINI,
        SEARCH_ENGINE_STARTER_PACK_PAGE,
        SEARCH_ENGINE_STARTER_PACK_AI_MODE,
    });

}  // namespace

SerpClassifier::SerpClassifier(TemplateURLService* template_url_service)
    : template_url_service_(template_url_service) {
  if (template_url_service_) {
    template_url_service->Load();
  }
}

SerpClassifier::~SerpClassifier() = default;

std::optional<SearchEngineType> SerpClassifier::Classify(const GURL& url) {
  if (!base::FeatureList::IsEnabled(kSerpMetricsFeature)) {
    return std::nullopt;
  }

  if (!template_url_service_ || !template_url_service_->loaded()) {
    return std::nullopt;
  }

  // Strip the port before classification. Search engine template search URLs
  // never include explicit ports, and test servers use random ones. Google
  // tests handle this via `switches::kIgnoreGooglePortNumbers`, but that switch
  // is Google-specific and does not apply to other hosts.
  GURL::Replacements url_replacements;
  url_replacements.ClearPort();
  GURL normalized_url = url.ReplaceComponents(url_replacements);

  if (std::optional<SearchEngineType> search_engine_type =
          MaybeClassifyTemplateUrlSearchEngine(normalized_url)) {
    return search_engine_type;
  }

  return MaybeClassifyPathBasedUrlSearchEngine(normalized_url);
}

///////////////////////////////////////////////////////////////////////////////

std::optional<SearchEngineType>
SerpClassifier::MaybeClassifyTemplateUrlSearchEngine(const GURL& url) {
  CHECK(template_url_service_);

  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForHost(url.GetHost());
  if (!template_url) {
    return std::nullopt;
  }

  if (template_url->type() != TemplateURL::NORMAL) {
    // Ignore non-standard search engines (e.g. extension/omnibox).
    return std::nullopt;
  }

  const SearchTermsData& search_terms_data =
      template_url_service_->search_terms_data();

  if (!template_url->IsSearchURL(url, search_terms_data)) {
    // Not a search URL.
    return std::nullopt;
  }

  // Some search engines (e.g. Qwant) perform two consecutive top-level
  // navigations for a single search, where the second navigation canonicalizes
  // the URL (e.g. by adding "t=web"). Since both appear as valid primary
  // main-frame commits, we dedupe consecutive navigations after normalizing the
  // URL via `KeepSearchTermsInURL` to avoid double counting.
  GURL normalized_url;
  template_url->KeepSearchTermsInURL(
      url, search_terms_data, /*keep_search_intent_params=*/false,
      /*normalize_search_terms=*/true, /*out_url=*/&normalized_url,
      /*out_search_terms=*/nullptr);
  if (last_normalized_url_ == normalized_url) {
    return std::nullopt;
  }
  last_normalized_url_ = normalized_url;

  const SearchEngineType search_engine_type =
      template_url->GetEngineType(search_terms_data);
  if (kDisallowList.contains(search_engine_type)) {
    return std::nullopt;
  }

  return search_engine_type;
}

std::optional<SearchEngineType>
SerpClassifier::MaybeClassifyPathBasedUrlSearchEngine(const GURL& url) const {
  if (base::MatchPattern(url.spec(), "https://chatgpt.com/c/*")) {
    return SearchEngineType::SEARCH_ENGINE_OTHER;
  }

  if (base::MatchPattern(url.spec(), "https://www.perplexity.ai/search/*") &&
      !base::MatchPattern(url.spec(),
                          "https://www.perplexity.ai/search/new/*")) {
    return SearchEngineType::SEARCH_ENGINE_OTHER;
  }

  // Yahoo SERPs are not recognized because their prepopulated templates are
  // query-based, while the sites now use path-based URLs.
  if (base::MatchPattern(url.spec(), "https://*.search.yahoo.com/search;*") ||
      base::MatchPattern(url.spec(), "https://search.yahoo.com/search;*")) {
    return SearchEngineType::SEARCH_ENGINE_YAHOO;
  }

  // Startpage SERPs are not recognized because their prepopulated templates are
  // query-based, while the sites now use path-based URLs.
  if (url == "https://startpage.com/sp/search") {
    return SearchEngineType::SEARCH_ENGINE_STARTPAGE;
  }

  return std::nullopt;
}

}  // namespace metrics
