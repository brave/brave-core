/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier.h"

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "url/gurl.h"

namespace serp_metrics {

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
  CHECK(template_url_service_);

  template_url_service->Load();
}

SerpClassifier::~SerpClassifier() = default;

bool SerpClassifier::IsSameSearchQuery(const GURL& lhs, const GURL& rhs) const {
  return NormalizeUrl(lhs) == NormalizeUrl(rhs);
}

std::optional<SearchEngineType> SerpClassifier::MaybeClassify(const GURL& url) {
  if (!template_url_service_->loaded()) {
    return std::nullopt;
  }

  const GURL normalized_url = NormalizeUrl(url);

  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForHost(normalized_url.GetHost());
  if (!template_url) {
    return std::nullopt;
  }

  if (template_url->type() != TemplateURL::NORMAL) {
    // Ignore non-standard search engines (e.g. extension/omnibox).
    return std::nullopt;
  }

  const SearchTermsData& search_terms_data =
      template_url_service_->search_terms_data();

  if (!template_url->IsSearchURL(normalized_url, search_terms_data)) {
    // Not a search URL.
    return std::nullopt;
  }

  const SearchEngineType search_engine_type =
      template_url->GetEngineType(search_terms_data);
  if (kDisallowList.contains(search_engine_type)) {
    return std::nullopt;
  }

  return search_engine_type;
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

  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForHost(normalized_url.GetHost());
  if (!template_url) {
    return normalized_url;
  }

  const SearchTermsData& search_terms_data =
      template_url_service_->search_terms_data();

  if (!template_url->IsSearchURL(normalized_url, search_terms_data)) {
    return normalized_url;
  }

  template_url->KeepSearchTermsInURL(normalized_url, search_terms_data,
                                     /*keep_search_intent_params=*/false,
                                     /*normalize_search_terms=*/true,
                                     /*out_url=*/&normalized_url,
                                     /*out_search_terms=*/nullptr);

  return normalized_url;
}

}  // namespace serp_metrics
