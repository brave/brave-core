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
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/common/web_discovery.mojom.h"
#include "url/gurl.h"

namespace web_discovery {

class URLExtractor;

struct PageScrapeResult {
  PageScrapeResult(GURL url, std::string id);
  ~PageScrapeResult();

  PageScrapeResult(const PageScrapeResult&) = delete;
  PageScrapeResult& operator=(const PageScrapeResult&) = delete;

  base::Value SerializeToValue();
  static std::unique_ptr<PageScrapeResult> FromValue(const base::Value& dict);

  GURL url;
  // A map of DOM selectors to list of scraped values embedded in a Dict.
  // Each dict contains arbitrary keys (defined in the patterns) to scraped
  // values.
  base::flat_map<std::string, std::vector<base::Value::Dict>> fields;
  std::string id;

  // Only available for non-strict scrapes with "searchQuery"/"widgetTitle"
  // scrape rules
  std::optional<std::string> query;
};

// Extracts attribute values from the page DOM for reporting purposes.
// ContentScraper utilizes the following techniques:
//
// a) Extraction within the current page in the renderer (via `ScrapePage`).
//    The `mojom::DocumentExtractor` is used to request attribute values
//    from the current DOM in the view. Typically, this is used to exact a
//    search query, and decide whether the page is worthy of investigation
//    and reporting.
// b) Parsing and extracting HTML from a double fetch. This follows
//    the extraction in a). Used to extract all other needed details
//    from the page i.e. search results. Uses a Rust library for DOM
//    operations, in respect of Rule of Two.
class ContentScraper {
 public:
  using PageScrapeResultCallback =
      base::OnceCallback<void(std::unique_ptr<PageScrapeResult>)>;

  static std::unique_ptr<ContentScraper> Create(
      const ServerConfigLoader* server_config_loader,
      const URLExtractor* url_extractor);

  virtual ~ContentScraper() = default;

  // For initial page scrape in renderer
  virtual void ScrapePage(const GURL& url,
                          bool is_strict_scrape,
                          mojom::DocumentExtractor* document_extractor,
                          PageScrapeResultCallback callback) = 0;
  // For subsequent double fetches after initial scrape
  virtual void ParseAndScrapePage(const GURL& url,
                                  bool is_strict_scrape,
                                  std::unique_ptr<PageScrapeResult> prev_result,
                                  std::string html,
                                  PageScrapeResultCallback callback) = 0;

  // For v2 patterns double fetch processing
  virtual void ParseAndScrapePageV2(const GURL& url,
                                    std::string response_body,
                                    PageScrapeResultCallback callback) = 0;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CONTENT_SCRAPER_H_
