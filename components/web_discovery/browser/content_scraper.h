/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CONTENT_SCRAPER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CONTENT_SCRAPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/web_discovery/browser/document_extractor/rs/src/lib.rs.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/common/web_discovery.mojom.h"
#include "url/gurl.h"

namespace web_discovery {

struct PageScrapeResult {
  PageScrapeResult(GURL url, std::string id);
  ~PageScrapeResult();

  PageScrapeResult(const PageScrapeResult&) = delete;
  PageScrapeResult& operator=(const PageScrapeResult&) = delete;

  base::Value SerializeToValue();
  static std::unique_ptr<PageScrapeResult> FromValue(const base::Value& dict);

  GURL url;
  base::flat_map<std::string, std::vector<base::Value::Dict>> fields;
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

  // For initial page scrape in renderer
  void ScrapePage(const GURL& url,
                  bool is_strict_scrape,
                  mojom::DocumentExtractor* document_extractor,
                  PageScrapeResultCallback callback);
  // For subsequent double fetches after initial scrape
  void ParseAndScrapePage(bool is_strict_scrape,
                          std::unique_ptr<PageScrapeResult> prev_result,
                          std::string html,
                          PageScrapeResultCallback callback);

 private:
  void ProcessStandardRule(const std::string& report_key,
                           const ScrapeRule& rule,
                           const std::string& root_selector,
                           const GURL& url,
                           PageScrapeResult* scrape_result);
  void OnScrapedElementAttributes(
      bool is_strict_scrape,
      std::unique_ptr<PageScrapeResult> scrape_result,
      PageScrapeResultCallback callback,
      std::vector<mojom::AttributeResultPtr> attribute_results);
  void OnRustElementAttributes(
      bool is_strict_scrape,
      std::unique_ptr<PageScrapeResult> scrape_result,
      PageScrapeResultCallback callback,
      rust::Vec<rust_document_extractor::AttributeResult> attribute_results);
  std::string MaybeExecuteRefineFunctions(const PatternsURLDetails* url_details,
                                          const std::string& root_selector,
                                          const std::string& report_key,
                                          std::string value);

  scoped_refptr<base::SequencedTaskRunner> pool_sequenced_task_runner_;

  raw_ptr<std::unique_ptr<PatternsGroup>> patterns_;

  base::WeakPtrFactory<ContentScraper> weak_ptr_factory_{this};
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CONTENT_SCRAPER_H_
