/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SAFE_BROWSING_BRAVE_GENERATED_SAFE_BROWSING_PREF_H_
#define BRAVE_BROWSER_SAFE_BROWSING_BRAVE_GENERATED_SAFE_BROWSING_PREF_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/extensions/api/settings_private/generated_pref.h"
#include "chrome/browser/safe_browsing/generated_safe_browsing_pref.h"
#include "chrome/common/extensions/api/settings_private.h"

class Profile;

namespace base {
class Value;
}  // namespace base

namespace safe_browsing {

// Adds Brave's "Limited protection" mode (Standard Safe Browsing with Brave's
// download protection off) to upstream's NO/STANDARD/ENHANCED set. Swapped in
// for the upstream class by the chromium_src generated_prefs.cc override.
class BraveGeneratedSafeBrowsingPref : public GeneratedSafeBrowsingPref {
 public:
  explicit BraveGeneratedSafeBrowsingPref(Profile* profile);
  ~BraveGeneratedSafeBrowsingPref() override;

  // Accepts Brave's Limited-protection value in addition to the upstream
  // levels. Returns PREF_NOT_MODIFIABLE if the underlying prefs are managed.
  extensions::settings_private::SetPrefResult SetPref(
      const base::Value* value) override;

  // May return Brave's Limited-protection value in addition to the upstream
  // levels.
  extensions::api::settings_private::PrefObject GetPrefObject() const override;

 private:
  const raw_ptr<Profile> profile_;
};

}  // namespace safe_browsing

#endif  // BRAVE_BROWSER_SAFE_BROWSING_BRAVE_GENERATED_SAFE_BROWSING_PREF_H_
