/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/reduce_language/browser/reduce_language_rule.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/types/expected.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace {

// reduce-language.json keys
const char kExclude[] = "exclude";

}  // namespace

namespace reduce_language {

ReduceLanguageRule::ReduceLanguageRule() {}

ReduceLanguageRule::~ReduceLanguageRule() = default;

// static
bool ReduceLanguageRule::GetURLPatternSetFromValue(
    const base::Value* value,
    extensions::URLPatternSet* result) {
  if (!value->is_list()) {
    return false;
  }
  std::string error;
  bool valid = result->Populate(
      value->GetList(), URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      false, &error);
  if (!valid) {
    DVLOG(1) << error;
  }
  return valid;
}

// static
void ReduceLanguageRule::RegisterJSONConverter(
    base::JSONValueConverter<ReduceLanguageRule>* converter) {
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kExclude, &ReduceLanguageRule::exclude_pattern_set_,
      GetURLPatternSetFromValue);
}

// static
// All eTLD+1 calculations for Reduce-Language should flow through here so they
// are consistent in their private registries configuration.
const std::string ReduceLanguageRule::GetETLDForReduceLanguage(
    const std::string& host) {
  return net::registry_controlled_domains::GetDomainAndRegistry(
      host, net::registry_controlled_domains::PrivateRegistryFilter::
                EXCLUDE_PRIVATE_REGISTRIES);
}

// static
base::expected<std::pair<std::vector<std::unique_ptr<ReduceLanguageRule>>,
                         base::flat_set<std::string>>,
               std::string>
ReduceLanguageRule::ParseRules(const std::string& contents) {
  if (contents.empty()) {
    return base::unexpected("Could not obtain reduce_language configuration");
  }
  absl::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    return base::unexpected("Failed to parse reduce_language configuration");
  }
  std::vector<std::string> excluded_hosts;
  std::vector<std::unique_ptr<ReduceLanguageRule>> rules;
  base::JSONValueConverter<ReduceLanguageRule> converter;
  for (base::Value& it : root->GetList()) {
    std::unique_ptr<ReduceLanguageRule> rule =
        std::make_unique<ReduceLanguageRule>();
    if (!converter.Convert(it, rule.get())) {
      continue;
    }
    for (const URLPattern& pattern : rule->exclude_pattern_set()) {
      if (!pattern.host().empty()) {
        const std::string etldp1 =
            ReduceLanguageRule::GetETLDForReduceLanguage(pattern.host());
        if (!etldp1.empty()) {
          excluded_hosts.push_back(std::move(etldp1));
        }
      }
    }
    rules.push_back(std::move(rule));
  }
  return std::pair<std::vector<std::unique_ptr<ReduceLanguageRule>>,
                   base::flat_set<std::string>>(std::move(rules),
                                                excluded_hosts);
}

bool ReduceLanguageRule::AppliesTo(const GURL& url) const {
  // If URL matches an explicitly excluded pattern, this exception rule applies.
  if (exclude_pattern_set_.MatchesURL(url)) {
    return true;
  }

  return false;
}

}  // namespace reduce_language
