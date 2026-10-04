// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/core/browser/psst_rule.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

// psst.json keys
constexpr char kInclude[] = "include";
constexpr char kExclude[] = "exclude";
constexpr char kName[] = "name";
constexpr char kVersion[] = "version";
constexpr char kUserScript[] = "user_script";
constexpr char kPolicyScript[] = "policy_script";

bool GetUrlPatternsFromValue(const base::Value* value,
                             std::vector<psst::PsstUrlPattern>* result) {
  if (!value->is_list()) {
    return false;
  }
  for (const auto& item : value->GetList()) {
    if (!item.is_string()) {
      return false;
    }
    auto pattern = psst::PsstUrlPattern::Create(item.GetString());
    if (!pattern) {
      DVLOG(1) << "Invalid psst URL pattern: " << item.GetString();
      return false;
    }
    result->push_back(std::move(*pattern));
  }
  return true;
}

bool GetFilePathFromValue(const base::Value* value, base::FilePath* result) {
  if (!value->is_string()) {
    return false;
  }
  const auto& val = value->GetString();
  *result = base::FilePath::FromASCII(val);
  return true;
}

}  // namespace

namespace psst {

// static
std::optional<PsstUrlPattern> PsstUrlPattern::Create(
    const std::string& pattern) {
  const size_t scheme_end = pattern.find(url::kStandardSchemeSeparator);
  if (scheme_end == std::string::npos) {
    return std::nullopt;
  }

  PsstUrlPattern result;
  result.scheme = pattern.substr(0, scheme_end);
  // Only https is supported (mirrors URLPattern::SCHEME_HTTPS).
  if (result.scheme != url::kHttpsScheme) {
    return std::nullopt;
  }

  const size_t host_start =
      scheme_end + std::string_view(url::kStandardSchemeSeparator).size();
  const size_t path_start = pattern.find('/', host_start);
  // A path component is required (e.g. the trailing "/*").
  if (path_start == std::string::npos) {
    return std::nullopt;
  }

  std::string host = pattern.substr(host_start, path_start - host_start);
  // A leading "*." matches the domain and all of its subdomains.
  static constexpr std::string_view kSubdomainPrefix = "*.";
  if (host.starts_with(kSubdomainPrefix)) {
    result.match_subdomains = true;
    host = host.substr(kSubdomainPrefix.size());
  }
  if (host.empty()) {
    return std::nullopt;
  }
  result.host = std::move(host);
  result.path = pattern.substr(path_start);
  return result;
}

bool PsstUrlPattern::Matches(const GURL& url) const {
  if (url.scheme() != scheme) {
    return false;
  }
  if (match_subdomains) {
    if (url.host() != host &&
        !url.host().ends_with(base::StrCat({".", host}))) {
      return false;
    }
  } else if (url.host() != host) {
    return false;
  }
  return base::MatchPattern(url.path(), path);
}

PsstRule::PsstRule() = default;
PsstRule::~PsstRule() = default;
PsstRule::PsstRule(const PsstRule& other) = default;

// static
void PsstRule::RegisterJSONConverter(
    base::JSONValueConverter<PsstRule>* converter) {
  converter->RegisterCustomValueField<std::vector<PsstUrlPattern>>(
      kInclude, &PsstRule::include_patterns_, GetUrlPatternsFromValue);
  converter->RegisterCustomValueField<std::vector<PsstUrlPattern>>(
      kExclude, &PsstRule::exclude_patterns_, GetUrlPatternsFromValue);
  converter->RegisterStringField(kName, &PsstRule::name_);
  converter->RegisterCustomValueField<base::FilePath>(
      kUserScript, &PsstRule::user_script_path_, GetFilePathFromValue);
  converter->RegisterCustomValueField<base::FilePath>(
      kPolicyScript, &PsstRule::policy_script_path_, GetFilePathFromValue);
  converter->RegisterIntField(kVersion, &PsstRule::version_);
}

// static
std::optional<std::vector<PsstRule>> PsstRule::ParseRules(
    const std::string& contents) {
  if (contents.empty()) {
    return std::nullopt;
  }
  std::optional<base::ListValue> root = base::JSONReader::ReadList(
      contents, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!root) {
    VLOG(1) << "PsstRule::ParseRules: invalid JSON";
    return std::nullopt;
  }
  std::vector<PsstRule> rules;
  base::JSONValueConverter<PsstRule> converter;
  for (base::Value& it : *root) {
    PsstRule rule = PsstRule();
    if (!converter.Convert(it, &rule)) {
      VLOG(1) << "PsstRule::ParseRules: invalid rule";
      continue;
    }
    rules.emplace_back(rule);
  }
  return rules;
}

bool PsstRule::ShouldInsertScript(const GURL& url) const {
  const auto matches = [&url](const PsstUrlPattern& pattern) {
    return pattern.Matches(url);
  };
  // If URL matches an explicitly excluded pattern, this rule does not
  // apply.
  if (std::ranges::any_of(exclude_patterns_, matches)) {
    return false;
  }
  // If URL does not match an explicitly included pattern, this rule does not
  // apply.
  if (!std::ranges::any_of(include_patterns_, matches)) {
    return false;
  }

  return true;
}

}  // namespace psst
