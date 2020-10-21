/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include "base/no_destructor.h"
#include "brave/browser/themes/brave_theme_helper.h"
#include "brave/browser/extensions/brave_theme_event_router.h"
#include "chrome/browser/profiles/profile.h"
#include "brave/browser/profiles/profile_util.h"

#if defined(OS_WIN)
#include "brave/browser/themes/brave_theme_helper_win.h"
#endif

namespace {

const ThemeHelper& GetBraveThemeHelper(Profile* profile) {
#if defined(OS_WIN)
  using BraveThemeHelper = BraveThemeHelperWin;
#endif
  // Because the helper is created as a NoDestructor static, we need separate
  // instances for regular, tor and guest profiles.
  if (profile->IsTor()) {
    static base::NoDestructor<std::unique_ptr<ThemeHelper>> dark_theme_helper(
        std::make_unique<BraveThemeHelper>());
    (static_cast<BraveThemeHelper*>(dark_theme_helper.get()->get()))
        ->set_is_tor();
    return **dark_theme_helper;
  } else if (brave::IsGuestProfile(profile)) {
    static base::NoDestructor<std::unique_ptr<ThemeHelper>> dark_theme_helper(
        std::make_unique<BraveThemeHelper>());
    (static_cast<BraveThemeHelper*>(dark_theme_helper.get()->get()))
        ->set_is_guest();
    return **dark_theme_helper;
  } else {
    static base::NoDestructor<std::unique_ptr<ThemeHelper>> theme_helper(
        std::make_unique<BraveThemeHelper>());
    return **theme_helper;
  }
}

}  // namespace

// Replace Chromium's ThemeHelper with BraveThemeHelper that is appropriate for
// the given profile. There should only be 3 static ThemeHelpers at most: the
// original Chromium one, and 2 Brave ones.
BraveThemeService::BraveThemeService(Profile* profile,
                                     const ThemeHelper& theme_helper)
    : ThemeService(profile, GetBraveThemeHelper(profile)) {
  brave_theme_event_router_.reset(
      new extensions::BraveThemeEventRouter(profile));
}

BraveThemeService::~BraveThemeService() = default;

void BraveThemeService::SetBraveThemeEventRouterForTesting(
    extensions::BraveThemeEventRouter* mock_router) {
  brave_theme_event_router_.reset(mock_router);
}
