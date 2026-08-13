/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_migration_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "base/check_deref.h"
#include "base/strings/strcat.h"
#include "components/content_settings/core/browser/content_settings_pref_provider.h"
#include "components/content_settings/core/browser/content_settings_rule.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

namespace content_settings::brave_content_settings_migration {

namespace {

std::unique_ptr<Rule> CloneRule(const Rule& original_rule) {
  return std::make_unique<Rule>(
      original_rule.primary_pattern, original_rule.secondary_pattern,
      original_rule.value.Clone(), original_rule.metadata.Clone());
}

// Returns whether |rule| is a JAVASCRIPT rule that Shields authored, as opposed
// to one the user created through the Chromium Site Settings UI. Shields wrote
// its per-site JS rules with a host pattern of the form "*://host/*" (produced
// by content_settings::CreateHostPattern()) paired with a wildcard secondary
// pattern; reconstructing that pattern from the rule's host and comparing lets
// us select only Shields-origin rules during migration.
bool IsShieldsAuthoredJavascriptRule(const Rule& rule) {
  if (rule.secondary_pattern != ContentSettingsPattern::Wildcard()) {
    return false;
  }
  const std::string host = rule.primary_pattern.GetHost();
  if (host.empty()) {
    return false;
  }
  return rule.primary_pattern == content_settings::CreateHostPattern(GURL(
                                     base::StrCat({"https://", host, "/"})));
}

}  // namespace

std::vector<std::unique_ptr<Rule>> GetShieldsAuthoredJavascriptRules(
    PrefProvider& provider) {
  std::vector<std::unique_ptr<Rule>> rules_to_migrate;
  auto rule_iterator = provider.GetRuleIterator(ContentSettingsType::JAVASCRIPT,
                                                /*off_the_record=*/false);
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    if (IsShieldsAuthoredJavascriptRule(*rule)) {
      rules_to_migrate.emplace_back(CloneRule(CHECK_DEREF(rule.get())));
    }
  }
  return rules_to_migrate;
}

}  // namespace content_settings::brave_content_settings_migration
