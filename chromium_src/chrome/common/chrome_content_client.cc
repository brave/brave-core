/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/chrome_content_client.h"

#define AddPlugins AddPlugins_ChromiumImpl

#include "src/chrome/common/chrome_content_client.cc"

#undef AddPlugins

void ChromeContentClient::AddPlugins(
    std::vector<content::ContentPluginInfo>* plugins) {
  AddPlugins_ChromiumImpl(plugins);
#if BUILDFLAG(ENABLE_PDF)
  auto iter = base::ranges::find_if(*plugins, [](const auto& plugin_info) {
    return plugin_info.name == "Chromium PDF Plugin";
  });
  if (iter == plugins->end()) {
    return;
  }
  auto& plugin_info = *iter;
  plugin_info.name = "Chrome PDF Plugin";
#endif  // BUILDFLAG(ENABLE_PDF)
}
