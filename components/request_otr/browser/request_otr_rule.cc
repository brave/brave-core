/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_rule.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/types/expected.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace {

// request-otr.json keys
constexpr char kInclude[] = "include";
constexpr char kExclude[] = "exclude";

// Removes trailing dot from |host_piece| if any.
// Copied from extensions/common/url_pattern.cc
std::string_view CanonicalizeHostForMatching(std::string_view host_piece) {
  if (base::EndsWith(host_piece, ".")) {
    host_piece.remove_suffix(1);
  }
  return host_piece;
}

}  // namespace

namespace request_otr {

RequestOTRRule::RequestOTRRule() {}

RequestOTRRule::~RequestOTRRule() = default;

// static
bool RequestOTRRule::GetURLPatternSetFromValue(
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
void RequestOTRRule::RegisterJSONConverter(
    base::JSONValueConverter<RequestOTRRule>* converter) {
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kInclude, &RequestOTRRule::include_pattern_set_,
      GetURLPatternSetFromValue);
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kExclude, &RequestOTRRule::exclude_pattern_set_,
      GetURLPatternSetFromValue);
}

// static
// All eTLD+1 calculations for Request-OTR-Tab should flow through here so they
// are consistent in their private registries configuration.
const std::string RequestOTRRule::GetETLDForRequestOTR(
    const std::string& host) {
  std::string_view host_piece = CanonicalizeHostForMatching(host);
  return net::registry_controlled_domains::GetDomainAndRegistry(
      host_piece, net::registry_controlled_domains::PrivateRegistryFilter::
                      EXCLUDE_PRIVATE_REGISTRIES);
}

// static
base::expected<std::pair<std::vector<std::unique_ptr<RequestOTRRule>>,
                         base::flat_set<std::string>>,
               std::string>
RequestOTRRule::ParseRules(const std::string& contents) {
  if (contents.empty()) {
    return base::unexpected("Could not obtain request_otr configuration");
  }
  std::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    return base::unexpected("Failed to parse request_otr configuration");
  }
  std::pair<std::vector<std::unique_ptr<RequestOTRRule>>,
            base::flat_set<std::string>>
      result;
  auto& [rules, hosts] = result;
  base::JSONValueConverter<RequestOTRRule> converter;
  for (base::Value& it : root->GetList()) {
    std::unique_ptr<RequestOTRRule> rule = std::make_unique<RequestOTRRule>();
    if (!converter.Convert(it, rule.get())) {
      continue;
    }
    for (const URLPattern& pattern : rule->include_pattern_set()) {
      if (!pattern.host().empty()) {
        const std::string etldp1 =
            RequestOTRRule::GetETLDForRequestOTR(pattern.host());
        if (!etldp1.empty()) {
          hosts.insert(std::move(etldp1));
        }
      }
    }
    rules.push_back(std::move(rule));
  }
  return result;
}

bool RequestOTRRule::ShouldBlock(const GURL& url) const {
  // If URL matches an explicitly excluded pattern, this rule does not
  // apply.
  if (exclude_pattern_set_.MatchesURL(url)) {
    return false;
  }
  // If URL does not match an explicitly included pattern, this rule does not
  // apply.
  if (!include_pattern_set_.MatchesURL(url)) {
    return false;
  }

  return true;
}

}  // namespace request_otr
