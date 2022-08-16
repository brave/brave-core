/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/child/brave_runtime_features.h"

#include "brave/components/playlist/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "base/feature_list.h"
#include "brave/components/playlist/features.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#endif

namespace content {

void BraveSetRuntimeFeaturesDefaultsAndUpdateFromArgs(
    const base::CommandLine& command_line) {
#if BUILDFLAG(ENABLE_PLAYLIST)
  // When MediaSource API is enabled, we'll get "blob: " source url. We can't
  // download media files from it, so we disable this API when playlist is
  // eanbled.
  blink::WebRuntimeFeatures::EnableFeatureFromString(
      "MediaSourceStable",
      !base::FeatureList::IsEnabled(playlist::features::kPlaylist));
#endif
}

}  // namespace content
