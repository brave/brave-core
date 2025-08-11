/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

// For FirstRunServicePolicyBrowserTest.OpenFirstRunIfNeeded/BrowserSignin_0:
// Change test expectation: we don't allow FRE to change profile name to "Work",
// so adjust the expected string to be the default profile name
#define IDS_SIGNIN_DICE_WEB_INTERCEPT_ENTERPRISE_PROFILE_NAME_SAVE \
  IDS_SIGNIN_DICE_WEB_INTERCEPT_ENTERPRISE_PROFILE_NAME
#undef IDS_SIGNIN_DICE_WEB_INTERCEPT_ENTERPRISE_PROFILE_NAME
#define IDS_SIGNIN_DICE_WEB_INTERCEPT_ENTERPRISE_PROFILE_NAME \
  IDS_PROFILE_MENU_BRAVE_PLACEHOLDER_PROFILE_NAME

#include <chrome/browser/ui/startup/first_run_service_browsertest.cc>
#undef IDS_SIGNIN_DICE_WEB_INTERCEPT_ENTERPRISE_PROFILE_NAME
#define IDS_SIGNIN_DICE_WEB_INTERCEPT_ENTERPRISE_PROFILE_NAME \
  IDS_SIGNIN_DICE_WEB_INTERCEPT_ENTERPRISE_PROFILE_NAME_SAVE
#undef IDS_SIGNIN_DICE_WEB_INTERCEPT_ENTERPRISE_PROFILE_NAME_SAVE
