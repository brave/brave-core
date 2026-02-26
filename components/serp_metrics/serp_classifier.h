/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_

#include <memory>
#include <optional>

#include "components/search_engines/search_engine_type.h"

class GURL;
class TemplateURL;

namespace serp_metrics {

// SerpClassifier determines whether a URL is a search engine results page and,
// if so, identifies the corresponding search engine.

class SerpClassifier final {
 public:
  SerpClassifier() = default;
  ~SerpClassifier() = default;

  SerpClassifier(const SerpClassifier&) = delete;
  SerpClassifier& operator=(const SerpClassifier&) = delete;

  // Returns `true` if `lhs` and `rhs` represent the same search results page.
  bool IsSameSearchQuery(const GURL& lhs, const GURL& rhs) const;

  // Returns the corresponding search engine type if `url` is a SERP. Returns
  // `std::nullopt` if `url` is not a SERP.
  std::optional<SearchEngineType> MaybeClassify(const GURL& url);

 private:
  // Normalizes a URL so equivalent search results pages compare equal.
  GURL NormalizeUrl(const GURL& url) const;

  // Returns a `TemplateURL` if `url` matches the search engine results page for
  // any prepopulated engine in the allow list.
  std::unique_ptr<TemplateURL> MaybeGetTemplateUrl(const GURL& url) const;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_
