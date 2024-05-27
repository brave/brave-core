/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CONTENT_SCRAPER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CONTENT_SCRAPER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/common/web_discovery.mojom.h"
#include "url/gurl.h"

namespace web_discovery {

struct PageScrapeResult {
  PageScrapeResult();
  ~PageScrapeResult();

  PageScrapeResult(const PageScrapeResult&) = delete;
  PageScrapeResult& operator=(const PageScrapeResult&) = delete;

  base::flat_map<std::string, std::vector<std::string>> fields;
  std::string id;
};

class ContentScraper {
 public:
  using PageScrapeResultCallback =
      base::OnceCallback<void(std::unique_ptr<PageScrapeResult>)>;

  explicit ContentScraper(std::unique_ptr<PatternsGroup>* patterns);
  ~ContentScraper();

  ContentScraper(const ContentScraper&) = delete;
  ContentScraper& operator=(const ContentScraper&) = delete;

  const PatternsURLDetails* GetMatchingURLPattern(const GURL& url,
                                                  bool is_strict_scrape);
  void ScrapePage(const PatternsURLDetails* url_details,
                  const GURL& url,
                  mojom::DocumentExtractor* document_extractor,
                  PageScrapeResultCallback callback);

 private:
  void OnElementAttributes(
      std::unique_ptr<PageScrapeResult> scrape_result,
      PageScrapeResultCallback callback,
      std::vector<mojom::AttributeResultPtr> attribute_results);

  raw_ptr<std::unique_ptr<PatternsGroup>> patterns_;

  base::WeakPtrFactory<ContentScraper> weak_ptr_factory_{this};
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CONTENT_SCRAPER_H_
