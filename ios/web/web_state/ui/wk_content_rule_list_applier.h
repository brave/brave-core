// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_WEB_STATE_UI_WK_CONTENT_RULE_LIST_APPLIER_H_
#define BRAVE_IOS_WEB_WEB_STATE_UI_WK_CONTENT_RULE_LIST_APPLIER_H_

#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/ios/web/web_state/ui/wk_content_rule_list_registry.h"

@class WKContentRuleList;
@class WKUserContentController;

namespace web {

// Installs the rule lists compiled by a WKContentRuleListRegistry on a
// WKUserContentController, and keeps them in sync as lists are updated or
// removed.
//
// This class is not thread-safe and should only be accessed on the UI thread.
class WKContentRuleListApplier : public WKContentRuleListRegistry::Observer {
 public:
  explicit WKContentRuleListApplier(WKContentRuleListRegistry& registry);
  ~WKContentRuleListApplier() override;

  WKContentRuleListApplier(const WKContentRuleListApplier&) = delete;
  WKContentRuleListApplier& operator=(const WKContentRuleListApplier&) = delete;

  // Sets the WKUserContentController the rule lists are installed on. The
  // lists are uninstalled from the previous controller, if any.
  void SetUserContentController(
      WKUserContentController* user_content_controller);

  // WKContentRuleListRegistry::Observer:
  void OnRuleListUpdated(const WKContentRuleListRegistry::RuleListKey& key,
                         WKContentRuleList* rule_list,
                         WKContentRuleList* previous_rule_list) override;
  void OnRuleListRemoved(const WKContentRuleListRegistry::RuleListKey& key,
                         WKContentRuleList* rule_list) override;

 private:
  // Installs all compiled lists of `registry_` on `user_content_controller_`.
  void InstallAllRuleLists();

  // Uninstalls all compiled lists of `registry_` from
  // `user_content_controller_`.
  void UninstallAllRuleLists();

  SEQUENCE_CHECKER(sequence_checker_);

  // Weak as the registry is owned by the BrowserState, which may destroy it
  // before this.
  base::WeakPtr<WKContentRuleListRegistry> registry_;

  // Weak as the controller is owned by the WKWebViewConfiguration.
  __weak WKUserContentController* user_content_controller_ = nil;
};

}  // namespace web

#endif  // BRAVE_IOS_WEB_WEB_STATE_UI_WK_CONTENT_RULE_LIST_APPLIER_H_
