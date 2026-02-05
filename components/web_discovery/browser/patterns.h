/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PATTERNS_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PATTERNS_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "url/gurl.h"

namespace re2 {
class RE2;
}  // namespace re2

namespace web_discovery {

enum class ScrapeRuleType {
  // Will retrieve a value not defined in the DOM, such as the client country
  // code or the current url.
  kStandard,
  // If the following two types are used for a rule, the value will be marked
  // as the search query, which will be used for privacy checks.
  kSearchQuery,
  kWidgetTitle,
  // All other rules should have this type. No special processing will be
  // performed.
  kOther
};
enum class PayloadRuleType {
  // Coupled with the `kClustered` result type.
  // All instances of a given attribute will be grouped into a single payload.
  kQuery,
  // Coupled with the `kSingle` result type.
  // Each instance of a given attribute will have its own payload.
  kSingle
};
enum class PayloadResultType {
  // Coupled with the `kSingle` rule type.
  kSingle,
  // Coupled with the `kClustered` rule type.
  kClustered,
  // Currently unsupported/ignored.
  kCustom
};

// Contains functions for refining the scraped value. The inner vector
// contains the function name and arguments for the function.
using RefineFunctionList = std::vector<base::ListValue>;

// Defines rule for scraping an attribute from a given selected element.
struct ScrapeRule {
  ScrapeRule();
  ~ScrapeRule();

  ScrapeRule(const ScrapeRule&) = delete;
  ScrapeRule& operator=(const ScrapeRule&) = delete;

  // An optional selector for an element within the current selected element.
  // The attribute will be retrieved from the embedded element.
  std::optional<std::string> sub_selector;
  ScrapeRuleType rule_type;
  // The name of the attribute to retrieve for a DOM element.
  std::string attribute;
  // Functions used to refine the retrieved value. See the "func ids" defined
  // in content_scraper.cc for all possible functions.
  RefineFunctionList functions_applied;
};

// A map of keys (arbitrary IDs used for storing the scraped result) to scrape
// rules.
using ScrapeRuleGroup =
    base::flat_map<std::string, std::unique_ptr<ScrapeRule>>;

// A rule for provided a single key/value pair within the submission payload.
struct PayloadRule {
  PayloadRule();
  ~PayloadRule();

  PayloadRule(const PayloadRule&) = delete;
  PayloadRule& operator=(const PayloadRule&) = delete;

  // The DOM selector of the scraped attribute.
  std::string selector;
  // The arbitrary key associated with the scraped value.
  std::string key;
  // If set to true, an array-like Dict (each dict key is an index)
  // will be rendered.
  // Each value in the Dict will be a Dict containing all keys/values
  // associated with the selector. This is commonly used for listing search
  // results.
  bool is_join = false;
};

// Contains rules for generating a payload for submission.
struct PayloadRuleGroup {
  PayloadRuleGroup();
  ~PayloadRuleGroup();

  PayloadRuleGroup(const PayloadRuleGroup&) = delete;
  PayloadRuleGroup& operator=(const PayloadRuleGroup&) = delete;

  // An arbitrary ID for the rule group. Current, this isn't used in the
  // payload.
  std::string key;
  PayloadRuleType rule_type;
  PayloadResultType result_type;
  // The name of the "action" for the given payload.
  std::string action;
  // The rules for generating the fields within the payload.
  std::vector<PayloadRule> rules;
};

// Contains settings and rule groups associated with a particular URL.
struct PatternsURLDetails {
  PatternsURLDetails();
  ~PatternsURLDetails();

  PatternsURLDetails(const PatternsURLDetails&) = delete;
  PatternsURLDetails& operator=(const PatternsURLDetails&) = delete;

  // The regex used to match the URL in the address bar.
  std::unique_ptr<re2::RE2> url_regex;
  bool is_search_engine = false;
  // The two or three-letter arbitrary id associated with the site.
  std::string id;
  // The search path prefix used for constructing private search queries
  // for double fetching.
  std::optional<std::string> search_template_prefix;
  // The scraping rules for the site. A map of DOM selectors
  // to rule groups.
  base::flat_map<std::string, ScrapeRuleGroup> scrape_rule_groups;
  // The payload generation rules used for generating submissions
  // from scraped attributes.
  std::vector<PayloadRuleGroup> payload_rule_groups;
};

// The full "patterns" configuration provided by the Web Discovery server.
// The configuration provides rules for scraping certain pages.
struct PatternsGroup {
  PatternsGroup();
  ~PatternsGroup();

  PatternsGroup(const PatternsGroup&) = delete;
  PatternsGroup& operator=(const PatternsGroup&) = delete;

  // Checks URL against all URL regexes in either the "normal" or "strict" set,
  // and returns the URL details/rules if available.
  const PatternsURLDetails* GetMatchingURLPattern(const GURL& url,
                                                  bool is_strict_scrape) const;

  // A list of URLs and rules used for scraping pages in the renderer,
  // pre-"double fetch". These rules typically scrape simple attributes which
  // are used to determine whether a page is private (i.e. the search query).
  std::vector<PatternsURLDetails> normal_patterns;
  // A list of URLS and rules used for scraping contents from a "double fetch".
  // The rules are usually more involved than the "normal" rules. In the case of
  // search engine result pages, the rules will be used to retrieve the
  // search results and any other relevant details.
  std::vector<PatternsURLDetails> strict_patterns;
};

// Returns nullptr if parsing fails.
std::unique_ptr<PatternsGroup> ParsePatterns(std::string_view patterns_json);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PATTERNS_H_
