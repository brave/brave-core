/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "components/search_engines/search_engine_type.h"
#include "url/gurl.h"

class TemplateURLService;

namespace metrics {

// SerpClassifier classifies navigation URLs as search engine results pages
// (SERPs) and identifies the corresponding search engine type when applicable.

class SerpClassifier final {
 public:
  explicit SerpClassifier(TemplateURLService* template_url_service);
  ~SerpClassifier();

  SerpClassifier(const SerpClassifier&) = delete;
  SerpClassifier& operator=(const SerpClassifier&) = delete;

  // Returns the corresponding search engine type if `url` is a SERP.
  // Returns `std::nullopt` if `url` is not a SERP or is a repeated consecutive
  // navigation to the same canonical SERP URL.
  std::optional<SearchEngineType> Classify(const GURL& url);

 private:
  std::optional<SearchEngineType> MaybeClassifyTemplateUrlSearchEngine(
      const GURL& url);
  std::optional<SearchEngineType> MaybeClassifyPathBasedUrlSearchEngine(
      const GURL& url) const;

  const raw_ptr<TemplateURLService> template_url_service_;  // Not owned.

  std::optional<GURL> last_normalized_url_;
};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_CLASSIFIER_H_
