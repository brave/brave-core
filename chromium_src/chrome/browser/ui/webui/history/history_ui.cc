// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/resources/history/grit/brave_history_resources.h"
#include "brave/browser/resources/history/grit/brave_history_resources_map.h"
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/buildflags.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {

void BraveAddHistoryResources(content::WebUIDataSource* source) {
#if !BUILDFLAG(OPTIMIZE_WEBUI)
  for (size_t i = 0; i < kBraveHistoryResourcesSize; ++i) {
    source->AddResourcePath(kBraveHistoryResources[i].path,
                            kBraveHistoryResources[i].id);
  }
#endif
}

void BraveCustomizeHistoryDataSource(content::WebUIDataSource* source,
                                     Profile* profile) {
  NavigationBarDataProvider::Initialize(source, profile);
  BraveAddHistoryResources(source);
}

}  // namespace

#define BRAVE_CREATE_HISTORY_UI_HTML_SOURCE \
  BraveCustomizeHistoryDataSource(source, profile);

#include "src/chrome/browser/ui/webui/history/history_ui.cc"
#undef BRAVE_CREATE_HISTORY_UI_HTML_SOURCE
