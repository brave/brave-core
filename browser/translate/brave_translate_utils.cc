/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/translate/brave_translate_utils.h"

#include "brave/components/translate/core/common/brave_translate_features.h"
#include "brave/components/translate/core/common/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace translate {

bool IsTranslateExtensionEnabled(content::BrowserContext* context) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  return extensions::ExtensionRegistry::Get(context)
      ->enabled_extensions()
      .Contains(google_translate_extension_id);
#else
  return false;
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
}

bool ShouldOfferExtensionInstallation(content::BrowserContext* context) {
  if (!IsTranslateExtensionAvailable())
    return false;
  return !IsTranslateExtensionEnabled(context);
}

bool IsInternalTranslationEnabled(content::BrowserContext* context) {
  return !IsTranslateExtensionEnabled(context) && IsBraveTranslateGoAvailable();
}

}  // namespace translate
