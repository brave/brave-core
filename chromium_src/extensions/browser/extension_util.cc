/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/common/manifest_handlers/incognito_info.h"

#define BRAVE_IS_INCOGNITO_ENABLED \
  if (context->IsTor())            \
    return IsTorEnabled(extension_id, context);

#define IsSplitMode ForSplitModeCheck(context->IsTor())->IsSplitMode
#include "../../../../extensions/browser/extension_util.cc"
#undef IsSplitMode
#undef BRAVE_IS_INCOGNITO_ENABLED

namespace extensions {
namespace util {

bool IsTorEnabled(const std::string& extension_id,
                  content::BrowserContext* context) {
  const Extension* extension =
      ExtensionRegistry::Get(context)->GetExtensionById(
          extension_id, ExtensionRegistry::ENABLED);
  if (extension) {
    if (!CanBeIncognitoEnabled(extension))
      return false;
    // If this is an existing component extension we always allow it to
    // work in incognito mode.
    if (Manifest::IsComponentLocation(extension->location()))
      return true;
  }
  if (!ExtensionPrefs::Get(context)->IsIncognitoEnabled(extension_id))
    return false;
  return ExtensionPrefs::Get(context)->IsTorEnabled(extension_id);
}

}  // namespace util
}  // namespace extensions
