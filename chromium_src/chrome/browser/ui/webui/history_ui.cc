// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/browser/resources/history/grit/brave_history_resources.h"
#include "brave/browser/resources/history/grit/brave_history_resources_map.h"
#include "chrome/common/buildflags.h"
#include "content/public/browser/web_ui_data_source.h"


namespace {

void BraveCustomizeHistoryDataSource(content::WebUIDataSource* source) {
  NavigationBarDataProvider::Initialize(source);
#if !BUILDFLAG(OPTIMIZE_WEBUI)
  const std::string prefix = "brave/";
  for (size_t i = 0; i < kBraveHistoryResourcesSize; ++i) {
    std::string path = prefix;
    path += kBraveHistoryResources[i].name;
    source->AddResourcePath(path, kBraveHistoryResources[i].value);
  }
#endif
}

}  // namespace

#include "../../../../../chrome/browser/ui/webui/history_ui.cc"  // NOLINT

