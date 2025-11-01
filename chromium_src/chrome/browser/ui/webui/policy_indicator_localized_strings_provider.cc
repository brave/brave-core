/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/policy_indicator_localized_strings_provider.h"

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

#define IDS_CONTROLLED_SETTING_POLICY_SAVE IDS_CONTROLLED_SETTING_POLICY
#undef IDS_CONTROLLED_SETTING_POLICY
#define IDS_CONTROLLED_SETTING_POLICY IDS_BRAVE_CONTROLLED_SETTING_POLICY

#include <chrome/browser/ui/webui/policy_indicator_localized_strings_provider.cc>

#undef IDS_CONTROLLED_SETTING_POLICY
#define IDS_CONTROLLED_SETTING_POLICY IDS_CONTROLLED_SETTING_POLICY_SAVE
#undef IDS_CONTROLLED_SETTING_POLICY_SAVE
