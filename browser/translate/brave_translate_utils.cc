// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/translate/brave_translate_utils.h"

#include "brave/components/translate/core/browser/brave_translate_features.h"
#include "brave/components/translate/core/browser/buildflags.h"
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
