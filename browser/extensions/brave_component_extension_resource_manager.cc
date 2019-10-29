/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_extension_resource_manager.h"

#include "brave/components/brave_extension/grit/brave_extension_generated_map.h"
#include "brave/components/brave_extension/grit/brave_extension_resources_map.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_generated_map.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources_map.h"
#include "brave/components/brave_wayback_machine/grit/brave_wayback_machine_generated_map.h"
#include "brave/components/brave_wayback_machine/grit/brave_wayback_machine_resources_map.h"

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/resources/extension/grit/brave_rewards_extension_resources_map.h"
#include "brave/components/brave_rewards/resources/extension/grit/brave_rewards_panel_generated_map.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "brave/components/brave_sync/grit/brave_sync_generated_map.h"
#include "brave/components/brave_sync/grit/brave_sync_resources_map.h"
#endif

namespace extensions {

BraveComponentExtensionResourceManager::
BraveComponentExtensionResourceManager() {
  AddComponentResourceEntries(
      kBraveExtension,
      kBraveExtensionSize);

  AddComponentResourceEntries(
      kBraveExtensionGenerated,
      kBraveExtensionGeneratedSize);
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  AddComponentResourceEntries(
      kBraveRewardsExtensionResources,
      kBraveRewardsExtensionResourcesSize);

  AddComponentResourceEntries(
      kBraveRewardsPanelGenerated,
      kBraveRewardsPanelGeneratedSize);
#endif
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  AddComponentResourceEntries(
      kBraveSyncResources,
      kBraveSyncResourcesSize);

  AddComponentResourceEntries(
      kBraveSyncGenerated,
      kBraveSyncGeneratedSize);
#endif

  AddComponentResourceEntries(
      kBraveWebtorrentResources,
      kBraveWebtorrentResourcesSize);

  AddComponentResourceEntries(
      kBraveWebtorrentGenerated,
      kBraveWebtorrentGeneratedSize);

  AddComponentResourceEntries(
      kBraveWaybackMachineResources,
      kBraveWaybackMachineResourcesSize);

  AddComponentResourceEntries(
      kBraveWaybackMachineGenerated,
      kBraveWaybackMachineGeneratedSize);
}

BraveComponentExtensionResourceManager::
~BraveComponentExtensionResourceManager() {}

}  // namespace extensions
