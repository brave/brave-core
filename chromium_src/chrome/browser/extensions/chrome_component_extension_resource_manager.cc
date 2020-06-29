/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/chrome_component_extension_resource_manager.h"

#include "brave/components/brave_extension/grit/brave_extension_generated_map.h"
#include "brave/components/brave_extension/grit/brave_extension_resources_map.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_generated_map.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources_map.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/resources/extension/grit/brave_rewards_extension_resources_map.h"
#include "brave/components/brave_rewards/resources/extension/grit/brave_rewards_panel_generated_map.h"
#endif

#define BRAVE_CHROME_COMPONENT_EXTENSION_RESOURCE_MANAGER_DATA \
  void AddBraveResources();

#define BRAVE_CHROME_COMPONENT_EXTENSION_RESOURCE_MANAGER_DATA_DATA \
  AddBraveResources();

#include "../../../../../chrome/browser/extensions/chrome_component_extension_resource_manager.cc"
#undef BRAVE_CHROME_COMPONENT_EXTENSION_RESOURCE_MANAGER_DATA_DATA
#undef BRAVE_CHROME_COMPONENT_EXTENSION_RESOURCE_MANAGER_DATA

namespace extensions {

void ChromeComponentExtensionResourceManager::Data::AddBraveResources() {
  AddComponentResourceEntries(kBraveExtension, kBraveExtensionSize);
  AddComponentResourceEntries(kBraveExtensionGenerated,
                              kBraveExtensionGeneratedSize);
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  AddComponentResourceEntries(kBraveRewardsExtensionResources,
                              kBraveRewardsExtensionResourcesSize);
  AddComponentResourceEntries(kBraveRewardsPanelGenerated,
                              kBraveRewardsPanelGeneratedSize);
#endif
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  AddComponentResourceEntries(kBraveWebtorrentResources,
                              kBraveWebtorrentResourcesSize);
  AddComponentResourceEntries(kBraveWebtorrentGenerated,
                              kBraveWebtorrentGeneratedSize);
#endif
}

}  // namespace extensions
