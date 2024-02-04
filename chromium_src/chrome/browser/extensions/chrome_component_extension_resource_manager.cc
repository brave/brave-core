/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/chrome_component_extension_resource_manager.h"

#include "brave/components/brave_extension/grit/brave_extension_generated_map.h"
#include "brave/components/brave_extension/grit/brave_extension_resources_map.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/simple_extension/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_generated_map.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources_map.h"
#endif

#if BUILDFLAG(ENABLE_SIMPLE_EXTENSION)
#include "brave/components/simple_extension/grit/simple_extension_generated_map.h"
#include "brave/components/simple_extension/grit/simple_extension_resources_map.h"
#endif


#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#define BRAVE_WEBTORRENT_RESOURCES                            \
  AddComponentResourceEntries(kBraveWebtorrentResources,      \
                              kBraveWebtorrentResourcesSize); \
  AddComponentResourceEntries(kBraveWebtorrentGenerated,      \
                              kBraveWebtorrentGeneratedSize);
#else
#define BRAVE_WEBTORRENT_RESOURCES
#endif

#if BUILDFLAG(ENABLE_SIMPLE_EXTENSION)
#define SIMPLE_EXTENSION_RESOURCES                            \
  AddComponentResourceEntries(kSimpleExtensionResources,      \
                              kSimpleExtensionResourcesSize); \
  AddComponentResourceEntries(kSimpleExtensionGenerated,      \
                              kSimpleExtensionGeneratedSize);
#else
#define SIMPLE_EXTENSION_RESOURCES
#endif


#define BRAVE_CHROME_COMPONENT_EXTENSION_RESOURCE_MANAGER_DATA_DATA  \
  AddComponentResourceEntries(kBraveExtension, kBraveExtensionSize); \
  AddComponentResourceEntries(kBraveExtensionGenerated,              \
                              kBraveExtensionGeneratedSize);         \
  BRAVE_WEBTORRENT_RESOURCES \
  SIMPLE_EXTENSION_RESOURCES

#include "src/chrome/browser/extensions/chrome_component_extension_resource_manager.cc"
#undef BRAVE_CHROME_COMPONENT_EXTENSION_RESOURCE_MANAGER_DATA_DATA
#undef BRAVE_WEBTORRENT_RESOURCES
#undef SIMPLE_EXTENSION_RESOURCES
