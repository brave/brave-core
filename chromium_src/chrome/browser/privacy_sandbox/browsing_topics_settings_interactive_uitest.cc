/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stddef.h>

namespace chrome {
inline constexpr char kPrivacySandboxManageTopicsURL[] =
    "chrome://settings/adPrivacy/interests/manage";
inline constexpr char kPrivacySandboxAdTopicsURL[] =
    "chrome://settings/adPrivacy/interests";
}  // namespace chrome

#define StartWithTwoBlockedTopics DISABLED_StartWithTwoBlockedTopics
#define BlockFirstTopicOnManageTopicsPage \
  DISABLED_BlockFirstTopicOnManageTopicsPage
#define UnblockOneTopicOnAdTopicsPage DISABLED_UnblockOneTopicOnAdTopicsPage
#define ConfirmDefaultIconIsNotUsed DISABLED_ConfirmDefaultIconIsNotUsed
#include "src/chrome/browser/privacy_sandbox/browsing_topics_settings_interactive_uitest.cc"
#undef StartWithTwoBlockedTopics
#undef BlockFirstTopicOnManageTopicsPage
#undef UnblockOneTopicOnAdTopicsPage
#undef ConfirmDefaultIconIsNotUsed
