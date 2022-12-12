/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_PREFS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_PREFS_H_

// This is done to allow the same renaming in
// chromium_src/chrome/browser/prefs/browser_prefs.cc
#define MigrateObsoleteProfilePrefs MigrateObsoleteProfilePrefs_ChromiumImpl
#define TranslatePrefs TranslatePrefs_ChromiumImpl
#include "src/components/translate/core/browser/translate_prefs.h"
#undef TranslatePrefs
#undef MigrateObsoleteProfilePrefs

namespace translate {
class TranslatePrefs : public TranslatePrefs_ChromiumImpl {
 public:
  using TranslatePrefs_ChromiumImpl::TranslatePrefs_ChromiumImpl;

  // Override to control by Brave features. No virtual because TranslatePrefs
  // doesn't have a virtual dtor and the method isn't used inside the impl.
  bool ShouldAutoTranslate(base::StringPiece source_language,
                           std::string* target_language);
};
}  // namespace translate

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_PREFS_H_
