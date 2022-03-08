/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/rewards_sync_internals/rewards_sync_internals_ui.h"

#include "brave/browser/ui/webui/rewards_sync_internals/rewards_sync_internals_message_handler.h"
#include "chrome/browser/ui/webui/sync_internals/sync_internals_message_handler.h"
#include "chrome/browser/ui/webui/sync_internals/sync_internals_ui.h"

#define kChromeUISyncInternalsHost kChromeUIRewardsSyncInternalsHost
#define SyncInternalsMessageHandler RewardsSyncInternalsMessageHandler
#define SyncInternalsUI RewardsSyncInternalsUI
#include "chrome/browser/ui/webui/sync_internals/sync_internals_ui.cc"
#undef SyncInternalsUI
#undef RewardsSyncInternalsMessageHandler
#undef kChromeUISyncInternalsHost
