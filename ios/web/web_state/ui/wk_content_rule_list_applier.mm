// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/web_state/ui/wk_content_rule_list_applier.h"

#include <WebKit/WebKit.h>

namespace web {

WKContentRuleListApplier::WKContentRuleListApplier(
    WKContentRuleListRegistry& registry)
    : registry_(registry.AsWeakPtr()) {
  registry.AddObserver(this);
}

WKContentRuleListApplier::~WKContentRuleListApplier() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (registry_) {
    registry_->RemoveObserver(this);
  }
}

void WKContentRuleListApplier::SetUserContentController(
    WKUserContentController* user_content_controller) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UninstallAllRuleLists();
  user_content_controller_ = user_content_controller;
  InstallAllRuleLists();
}

void WKContentRuleListApplier::OnRuleListUpdated(
    const WKContentRuleListRegistry::RuleListKey& key,
    WKContentRuleList* rule_list,
    WKContentRuleList* previous_rule_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (previous_rule_list) {
    [user_content_controller_ removeContentRuleList:previous_rule_list];
  }
  [user_content_controller_ addContentRuleList:rule_list];
}

void WKContentRuleListApplier::OnRuleListRemoved(
    const WKContentRuleListRegistry::RuleListKey& key,
    WKContentRuleList* rule_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  [user_content_controller_ removeContentRuleList:rule_list];
}

void WKContentRuleListApplier::InstallAllRuleLists() {
  if (!registry_) {
    return;
  }
  for (const auto& [key, rule_list] : registry_->GetCompiledRuleLists()) {
    [user_content_controller_ addContentRuleList:rule_list];
  }
}

void WKContentRuleListApplier::UninstallAllRuleLists() {
  if (!registry_) {
    return;
  }
  for (const auto& [key, rule_list] : registry_->GetCompiledRuleLists()) {
    [user_content_controller_ removeContentRuleList:rule_list];
  }
}

}  // namespace web
