/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/web_state/ui/wk_content_rule_list_util.h"

// Overrides Chrome's Content Blocker Rule List
// So it doesn't block WebUIs from opening external URLs
#define CreateLocalBlockingJsonRuleList \
  CreateLocalBlockingJsonRuleList() {   \
    return @"";                         \
  }                                     \
  NSString* CreateLocalBlockingJsonRuleList_ChromiumImpl

#include <ios/web/web_state/ui/wk_content_rule_list_util.mm>

#undef CreateLocalBlockingJsonRuleList
