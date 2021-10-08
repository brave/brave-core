// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/translate/brave_translate_utils.h"

#include "brave/components/translate/core/browser/brave_translate_features.h"
#include "brave/components/translate/core/browser/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"

namespace translate {

bool TranslateExtensionIsEnabled(content::BrowserContext* context) {
  return extensions::ExtensionRegistry::Get(context)
      ->enabled_extensions()
      .Contains(google_translate_extension_id);
}

bool ShouldOfferExtensionInstation(content::BrowserContext* context) {
  if (!IsTranslateExtensionAvailable())
    return false;
  return !TranslateExtensionIsEnabled(context);
}

bool InternalTranslationIsEnabled(content::BrowserContext* context) {
  return !TranslateExtensionIsEnabled(context) && IsBraveTranslateGoAvailable();
}

}  // namespace translate
