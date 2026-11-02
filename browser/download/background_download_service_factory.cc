/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"

namespace brave_shields {

AdBlockSubscriptionServiceManager* GetAdBlockSubscriptionServiceManager() {
  return g_brave_browser_process->ad_block_service()
      ->subscription_service_manager();
}

}  // namespace brave_shields
