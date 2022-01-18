/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_sync_service_factory.h"

#include "brave/browser/brave_rewards/rewards_chrome_sync_client.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "chrome/browser/sync/sync_service_factory.h"

#define ChromeSyncClient RewardsChromeSyncClient
#define SyncServiceFactory RewardsSyncServiceFactory
#include "chrome/browser/sync/sync_service_factory.cc"
#undef SyncServiceFactory
