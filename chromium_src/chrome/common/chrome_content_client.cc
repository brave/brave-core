/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/chrome_content_client.h"

#include <algorithm>

#include "content/public/common/webplugininfo.h"
#include "pdf/buildflags.h"

namespace {

void RenameChromiumPdfPluginToChrome(
    std::vector<content::WebPluginInfo>* plugins) {
#if BUILDFLAG(ENABLE_PDF)
  auto iter = std::ranges::find_if(*plugins, [](const auto& plugin_info) {
    return plugin_info.name == u"Chromium PDF Plugin";
  });
  if (iter == plugins->end()) {
    return;
  }
  iter->name = u"Chrome PDF Plugin";
#endif  // BUILDFLAG(ENABLE_PDF)
}

}  // namespace

#include <chrome/common/chrome_content_client.cc>
