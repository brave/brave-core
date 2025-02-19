/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/chrome_component_extension_resource_manager.h"

#include "brave/components/brave_extension/grit/brave_extension_generated_map.h"
#include "brave/components/brave_extension/grit/brave_extension_resources_map.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_generated_map.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources_map.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#define BRAVE_WEBTORRENT_RESOURCES                        \
  AddComponentResourceEntries(kBraveWebtorrentResources); \
  AddComponentResourceEntries(kBraveWebtorrentGenerated);
#else
#define BRAVE_WEBTORRENT_RESOURCES
#endif

#define BRAVE_CHROME_COMPONENT_EXTENSION_RESOURCE_MANAGER_DATA_DATA \
  AddComponentResourceEntries(kBraveExtension);                     \
  AddComponentResourceEntries(kBraveExtensionGenerated);            \
  BRAVE_WEBTORRENT_RESOURCES

#include "src/chrome/browser/extensions/chrome_component_extension_resource_manager.cc"
#undef BRAVE_CHROME_COMPONENT_EXTENSION_RESOURCE_MANAGER_DATA_DATA
#undef BRAVE_WEBTORRENT_RESOURCES
