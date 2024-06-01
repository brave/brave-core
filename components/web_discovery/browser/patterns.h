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

#include "url/gurl.h"

namespace re2 {
class RE2;
}  // namespace re2

namespace web_discovery {

enum class ScrapeRuleType { kStandard, kSearchQuery, kWidgetTitle, kOther };
enum class PayloadRuleType { kQuery, kSingle };
enum class PayloadResultType { kSingle, kClustered, kCustom };

struct ScrapeRule {
  ScrapeRule();
  ~ScrapeRule();

  ScrapeRule(const ScrapeRule&) = delete;
  ScrapeRule& operator=(const ScrapeRule&) = delete;

  std::string report_key;
  std::string selector;
  std::optional<std::string> sub_selector;
  ScrapeRuleType rule_type;
  std::string attribute;
};

struct ScrapeRuleGroup {
  ScrapeRuleGroup();
  ~ScrapeRuleGroup();

  ScrapeRuleGroup(const ScrapeRuleGroup&) = delete;
  ScrapeRuleGroup& operator=(const ScrapeRuleGroup&) = delete;

  std::string selector;
  std::vector<ScrapeRule> rules;
};

struct PayloadRule {
  PayloadRule();
  ~PayloadRule();

  PayloadRule(const PayloadRule&) = delete;
  PayloadRule& operator=(const PayloadRule&) = delete;

  std::string selector;
  std::string key;
  bool is_join = false;
};

struct PayloadRuleGroup {
  PayloadRuleGroup();
  ~PayloadRuleGroup();

  PayloadRuleGroup(const PayloadRuleGroup&) = delete;
  PayloadRuleGroup& operator=(const PayloadRuleGroup&) = delete;

  std::string key;
  PayloadRuleType rule_type;
  PayloadResultType result_type;
  std::string action;
  std::vector<PayloadRule> rules;
};

struct PatternsURLDetails {
  PatternsURLDetails();
  ~PatternsURLDetails();

  PatternsURLDetails(const PatternsURLDetails&) = delete;
  PatternsURLDetails& operator=(const PatternsURLDetails&) = delete;

  std::unique_ptr<re2::RE2> url_regex;
  bool is_search_engine;
  std::string id;
  std::vector<ScrapeRuleGroup> scrape_rule_groups;
  std::vector<PayloadRuleGroup> payload_rule_groups;
};

struct PatternsGroup {
  PatternsGroup();
  ~PatternsGroup();

  PatternsGroup(const PatternsGroup&) = delete;
  PatternsGroup& operator=(const PatternsGroup&) = delete;

  const PatternsURLDetails* GetMatchingURLPattern(const GURL& url,
                                                  bool is_strict_scrape);

  std::vector<PatternsURLDetails> normal_patterns;
  std::vector<PatternsURLDetails> strict_patterns;
};

std::unique_ptr<PatternsGroup> ParsePatterns(const std::string& patterns_json);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PATTERNS_H_
