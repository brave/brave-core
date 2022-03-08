/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_sync_internals_ui.h"

#include "brave/browser/ui/webui/brave_rewards_sync_internals_message_handler.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/webui/sync_internals/sync_internals_message_handler.h"
#include "chrome/browser/ui/webui/sync_internals/sync_internals_ui.h"
#include "chrome/common/webui_url_constants.h"

// sync_internals_ui.cc below uses chrome::kChromeUISyncInternalsHost
// internally.
namespace chrome {
const auto& kRewardsSyncInternalsHost = ::kRewardsSyncInternalsHost;
}

#define kChromeUISyncInternalsHost kRewardsSyncInternalsHost
#define SyncInternalsMessageHandler BraveRewardsSyncInternalsMessageHandler
#define SyncInternalsUI BraveRewardsSyncInternalsUI
#include "chrome/browser/ui/webui/sync_internals/sync_internals_ui.cc"
#undef kChromeUISyncInternalsHost
#undef SyncInternalsMessageHandler
#undef SyncInternalsUI
