/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "components/search_engines/search_engine_type.h"

class GURL;
class TemplateURLService;

namespace serp_metrics {

// SerpClassifier determines whether a URL is a search engine results page and,
// if so, identifies the corresponding search engine.

class SerpClassifier final {
 public:
  explicit SerpClassifier(TemplateURLService* template_url_service);
  ~SerpClassifier();

  SerpClassifier(const SerpClassifier&) = delete;
  SerpClassifier& operator=(const SerpClassifier&) = delete;

  // Returns `true` if `lhs` and `rhs` represent the same search results page.
  bool IsSameSearchQuery(const GURL& lhs, const GURL& rhs) const;

  // Returns the corresponding search engine type if `url` is a SERP. Returns
  // `std::nullopt` if `url` is not a SERP or if the navigation repeats the same
  // canonical SERP URL consecutively. This avoids double-counting.
  std::optional<SearchEngineType> MaybeClassify(const GURL& url);

 private:
  // Normalizes a URL so equivalent search results pages compare equal.
  GURL NormalizeUrl(const GURL& url) const;

  const raw_ptr<TemplateURLService> template_url_service_;  // Not owned.
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_
