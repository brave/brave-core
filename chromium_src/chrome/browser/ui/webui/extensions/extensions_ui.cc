/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/resources/extensions/grit/brave_extensions_resources.h"
#include "brave/browser/resources/extensions/grit/brave_extensions_resources_map.h"
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/common/buildflags.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"

namespace extensions {

namespace {

#if !BUILDFLAG(OPTIMIZE_WEBUI)
constexpr char kBraveGeneratedPath[] =
    "@out_folder@/gen/brave/browser/resources/extensions/preprocessed";
#endif

// Called from the original extension_ui.cc's CreateMdExtensionsSource via a
// patch.
void BraveAddExtensionsResources(content::WebUIDataSource* source) {
#if !BUILDFLAG(OPTIMIZE_WEBUI)
  for (size_t i = 0; i < kBraveExtensionsResourcesSize; ++i) {
    std::string path = kBraveExtensionsResources[i].path;
    if (path.rfind(kBraveGeneratedPath, 0) == 0) {
      path = path.substr(strlen(kBraveGeneratedPath));
    }
    source->AddResourcePath(path, kBraveExtensionsResources[i].id);
  }
#endif
  NavigationBarDataProvider::Initialize(source);
  source->AddLocalizedString("privateInfoWarning",
                             IDS_EXTENSIONS_BRAVE_PRIVATE_WARNING);
  source->AddLocalizedString("spanningInfoWarning",
                             IDS_EXTENSIONS_BRAVE_SPANNING_WARNING);
  source->AddLocalizedString("privateAndTorInfoWarning",
                             IDS_EXTENSIONS_BRAVE_PRIVATE_AND_TOR_WARNING);
}

}  // namespace

}  // namespace extensions

// These are defined in generated_resources.h, but since we are including it
// here the original extensions_ui.cc shouldn't include it again and the
// redefined values will be used.
#undef IDS_EXTENSIONS_ITEM_CHROME_WEB_STORE
#define IDS_EXTENSIONS_ITEM_CHROME_WEB_STORE \
  IDS_EXTENSIONS_BRAVE_ITEM_CHROME_WEB_STORE
#undef IDS_EXTENSIONS_ITEM_SOURCE_WEBSTORE
#define IDS_EXTENSIONS_ITEM_SOURCE_WEBSTORE \
  IDS_EXTENSIONS_BRAVE_ITEM_SOURCE_WEBSTORE

#include "../../../../../../../chrome/browser/ui/webui/extensions/extensions_ui.cc"
