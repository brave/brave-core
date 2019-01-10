/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"

// These are defined in generated_resources.h, but since we are including it
// here the original extensions_ui.cc shouldn't include it again and the
// redefined values will be used.
#undef IDS_MD_EXTENSIONS_ITEM_CHROME_WEB_STORE
#define IDS_MD_EXTENSIONS_ITEM_CHROME_WEB_STORE \
  IDS_MD_EXTENSIONS_BRAVE_ITEM_CHROME_WEB_STORE
#undef IDS_MD_EXTENSIONS_ITEM_SOURCE_WEBSTORE
#define IDS_MD_EXTENSIONS_ITEM_SOURCE_WEBSTORE \
  IDS_MD_EXTENSIONS_BRAVE_ITEM_SOURCE_WEBSTORE
#undef IDS_MD_EXTENSIONS_NO_INSTALLED_ITEMS
#define IDS_MD_EXTENSIONS_NO_INSTALLED_ITEMS \
  IDS_MD_EXTENSIONS_BRAVE_NO_INSTALLED_ITEMS

// Forward declarations needed due to extensions_ui.cc being patched with this
// function name.
namespace extensions {
namespace {
void BraveAddLocalizedStrings(content::WebUIDataSource* html_source);
} // namespace
} // namespace extensions

#include "../../../../../../chrome/browser/ui/webui/extensions/extensions_ui.cc"

namespace extensions {

namespace {

void BraveAddLocalizedStrings(content::WebUIDataSource* html_source) {
  html_source->AddLocalizedString("moreExtensions",
                                  IDS_MD_EXTENSIONS_BRAVE_MORE_EXTENSIONS);
}

} // namespace

} // namespace extensions
