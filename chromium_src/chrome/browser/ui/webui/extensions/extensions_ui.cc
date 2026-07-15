/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/buildflags.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "extensions/browser/extension_util.h"

namespace extensions {

namespace {

// Called from the original extensions_ui.cc's CreateAndAddExtensionsSource via
// a plaster.
void BraveAddExtensionsResources(content::WebUIDataSource* source,
                                 Profile* profile) {
  source->AddLocalizedString("privateInfoWarning",
                             IDS_EXTENSIONS_BRAVE_PRIVATE_WARNING);
  source->AddLocalizedString("spanningInfoWarning",
                             IDS_EXTENSIONS_BRAVE_SPANNING_WARNING);
  source->AddLocalizedString("privateAndTorInfoWarning",
                             IDS_EXTENSIONS_BRAVE_PRIVATE_AND_TOR_WARNING);
  source->AddLocalizedString("braveHosted", IDS_EXTENSIONS_BRAVE_HOSTED);

  const std::string mv2_extensions =
      base::JoinString(extensions_mv2::kPreconfiguredManifestV2Extensions, ",");
  source->AddString("braveHostedExtensions", mv2_extensions);
}

}  // namespace

}  // namespace extensions

#include <chrome/browser/ui/webui/extensions/extensions_ui.cc>  // IWYU pragma: export
