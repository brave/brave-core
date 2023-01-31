/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/search_engine/search_engine_constants.h"

#include "base/no_destructor.h"
#include "bat/ads/internal/common/search_engine/search_engine_info.h"
#include "bat/ads/internal/common/search_engine/search_engine_results_page_url_pattern_constants.h"
#include "bat/ads/internal/common/search_engine/search_engine_url_pattern_constants.h"

namespace ads {

const std::vector<SearchEngineInfo>& GetSearchEngines() {
  static const base::NoDestructor<std::vector<SearchEngineInfo>> kSearchEngines(
      {{"https://ask.com/", "https://ask.com/web", "q"},
       {"https://developer.mozilla.org/(.*)/",
        "https://developer.mozilla.org/(.*)/search", "q"},
       {"https://duckduckgo.com/", "https://duckduckgo.com/", "q"},
       {"https://fireball.de/", "https://fireball.de/search", "q"},
       {"https://github.com/", "https://github.com/search", "q"},
       {"https://infogalactic.com/", "https://infogalactic.com/info/(.*)", {}},
       {"https://search.brave.com/", "https://search.brave.com/search", "q"},
       {"https://stackoverflow.com/", "https://stackoverflow.com/search", "q"},
       {"https://swisscows.com/", "https://swisscows.com/web", "query"},
       {"https://twitter.com/explore/", "https://twitter.com/search", "q"},
       {"https://www.baidu.com/", "https://www.baidu.com/s", "wd"},
       {"https://www.bing.com/", "https://www.bing.com/search", "q"},
       {"https://www.dogpile.com/", "https://www.dogpile.com/serp", "q"},
       {"https://www.ecosia.org/", "https://www.ecosia.org/search", "q"},
       {"https://www.excite.com/", "https://results.excite.com/serp", "q"},
       {"https://www.findx.com/", "https://www.findx.com/search", "q"},
       {"https://www.gigablast.com/", "https://www.gigablast.com/search", "q"},
       {"https://www.lycos.com/", "https://search.lycos.com/web/", "q"},
       {"https://www.metacrawler.com/", "https://www.metacrawler.com/serp",
        "q"},
       {"https://www.petalsearch.com/", "https://www.petalsearch.com/search",
        "query"},
       {"https://www.qwant.com/", "https://www.qwant.com/", "q"},
       {"https://www.semanticscholar.org/",
        "https://www.semanticscholar.org/search", "q"},
       {"https://www.sogou.com/", "https://www.sogou.com/web", "query"},
       {"https://www.startpage.com/",
        "https://www.startpage.com/sp/search",
        {}},
       {"https://www.webcrawler.com/", "https://www.webcrawler.com/serp", "q"},
       {"https://www.wolframalpha.com/", "https://www.wolframalpha.com/input",
        "i"},
       {"https://www.youtube.com/", "https://www.youtube.com/results",
        "search_query"},
       {"https://yandex.com/", "https://yandex.com/search/", "text"},
       {GetAmazonUrlPattern(), GetAmazonResultsPageUrlPattern(), "k"},
       {GetGoogleUrlPattern(), GetGoogleResultsPageUrlPattern(), "q"},
       {GetMojeekUrlPattern(), GetMojeekResultsPageUrlPattern(), "q"},
       {GetWikipediaUrlPattern(), GetWikipediaResultsPageUrlPattern(), {}},
       {GetYahooUrlPattern(), GetYahooResultsPageUrlPattern(), "p"}});
  return *kSearchEngines;
}

}  // namespace ads
