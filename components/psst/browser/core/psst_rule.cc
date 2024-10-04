// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/types/expected.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace {

// psst.json keys
constexpr char kInclude[] = "include";
constexpr char kExclude[] = "exclude";
constexpr char kVersion[] = "version";
constexpr char kTestScript[] = "test_script";
constexpr char kPolicyScript[] = "policy_script";

bool GetURLPatternSetFromValue(const base::Value* value,
                               extensions::URLPatternSet* result) {
  if (!value->is_list()) {
    return false;
  }
  std::string error;
  bool valid = result->Populate(value->GetList(), URLPattern::SCHEME_HTTPS,
                                false, &error);
  if (!valid) {
    DVLOG(1) << error;
  }
  return valid;
}

bool GetFilePathFromValue(const base::Value* value, base::FilePath* result) {
  if (!value->is_string()) {
    return false;
  }
  auto val = value->GetString();
  *result = base::FilePath::FromASCII(val);
  return true;
}

}  // namespace

namespace psst {

PsstRule::PsstRule() = default;
PsstRule::~PsstRule() = default;
PsstRule::PsstRule(const PsstRule& other) {
  include_pattern_set_ = other.include_pattern_set_.Clone();
  exclude_pattern_set_ = other.exclude_pattern_set_.Clone();
  test_script_path_ = other.test_script_path_;
  policy_script_path_ = other.policy_script_path_;
  version_ = other.version_;
}

// static
void PsstRule::RegisterJSONConverter(
    base::JSONValueConverter<PsstRule>* converter) {
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kInclude, &PsstRule::include_pattern_set_, GetURLPatternSetFromValue);
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kExclude, &PsstRule::exclude_pattern_set_, GetURLPatternSetFromValue);
  converter->RegisterCustomValueField<base::FilePath>(
      kTestScript, &PsstRule::test_script_path_, GetFilePathFromValue);
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
  std::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    VLOG(1) << "PsstRule::ParseRules: invalid JSON";
    return std::nullopt;
  }
  std::vector<PsstRule> rules;
  base::JSONValueConverter<PsstRule> converter;
  for (base::Value& it : root->GetList()) {
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

}  // namespace psst
