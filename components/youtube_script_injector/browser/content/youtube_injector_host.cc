// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/youtube_script_injector/browser/content/youtube_injector_host.h"

#include "brave/components/youtube_script_injector/browser/content/youtube_tab_helper.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_registry.h"

namespace youtube_script_injector {
YouTubeInjectorHost::~YouTubeInjectorHost() = default;

YouTubeInjectorHost::YouTubeInjectorHost(const GURL& url) : url_(url) {}

void YouTubeInjectorHost::NativePipMode() {
  if (YouTubeRegistry::IsYouTubeDomain(url_)) {
    YouTubeTabHelper::EnterPipMode();
  }
}
}  // namespace youtube_script_injector
