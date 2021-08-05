/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_EXTENSIONS_PERMISSIONS_CHROME_PERMISSION_MESSAGE_RULES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_EXTENSIONS_PERMISSIONS_CHROME_PERMISSION_MESSAGE_RULES_H_

#define GetAllRules           \
  GetAllRules_ChromiumImpl(); \
  static std::vector<ChromePermissionMessageRule> GetAllRules
#include "../../../../../../chrome/common/extensions/permissions/chrome_permission_message_rules.h"
#undef GetAllRules

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_EXTENSIONS_PERMISSIONS_CHROME_PERMISSION_MESSAGE_RULES_H_
