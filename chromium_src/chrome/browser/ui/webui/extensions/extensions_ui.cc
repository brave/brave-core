/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/resources/extensions/grit/brave_extensions_resources.h"
#include "brave/browser/resources/extensions/grit/brave_extensions_resources_map.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/common/buildflags.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"

namespace extensions {

namespace {

// Called from the original extension_ui.cc's CreateMdExtensionsSource via a
// patch.
void BraveAddExtensionsResources(content::WebUIDataSource* source) {
#if !BUILDFLAG(OPTIMIZE_WEBUI)
  for (size_t i = 0; i < kBraveExtensionsResourcesSize; ++i) {
    source->AddResourcePath(kBraveExtensionsResources[i].name,
                            kBraveExtensionsResources[i].value);
  }
#endif
}

}  // namespace

}  // namespace extensions

// These are defined in generated_resources.h, but since we are including it
// here the original extensions_ui.cc shouldn't include it again and the
// redefined values will be used.
#undef IDS_MD_EXTENSIONS_ITEM_CHROME_WEB_STORE
#define IDS_MD_EXTENSIONS_ITEM_CHROME_WEB_STORE \
  IDS_MD_EXTENSIONS_BRAVE_ITEM_CHROME_WEB_STORE
#undef IDS_MD_EXTENSIONS_ITEM_SOURCE_WEBSTORE
#define IDS_MD_EXTENSIONS_ITEM_SOURCE_WEBSTORE \
  IDS_MD_EXTENSIONS_BRAVE_ITEM_SOURCE_WEBSTORE

#include "../../../../../../chrome/browser/ui/webui/extensions/extensions_ui.cc"  // NOLINT
