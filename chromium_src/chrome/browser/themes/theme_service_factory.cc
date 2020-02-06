/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/themes/theme_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/profiles/profile.h"

#if defined(OS_WIN)
#include "brave/browser/themes/brave_theme_helper_win.h"
#else
#include "brave/browser/themes/brave_theme_helper.h"
#endif

#if !defined(OS_LINUX)
#include "brave/browser/themes/brave_theme_service.h"
#endif

namespace {

// Forward declare the original function.
const ThemeHelper& GetThemeHelper();

const ThemeHelper& GetBraveThemeHelper(Profile* profile) {
#if defined(OS_WIN)
  using BraveThemeHelper = BraveThemeHelperWin;
#endif
  // Prevent unused function error for the original function (but also see
  // BRAVE_GETTHEME_HELPER below).
  const ThemeHelper& unused = GetThemeHelper();
  (void)unused;

  // Because the helper is created as a NoDestructor static, we need separate
  // instances for regular vs tor/guest profiles.
  if (brave::IsTorProfile(profile) || brave::IsGuestProfile(profile)) {
    static base::NoDestructor<std::unique_ptr<ThemeHelper>> dark_theme_helper(
        std::make_unique<BraveThemeHelper>());
    (static_cast<BraveThemeHelper*>(dark_theme_helper.get()->get()))
        ->SetTorOrGuest();
    return **dark_theme_helper;
  } else {
    static base::NoDestructor<std::unique_ptr<ThemeHelper>> theme_helper(
        std::make_unique<BraveThemeHelper>());
    return **theme_helper;
  }
}

}  // namespace

#if !defined(OS_LINUX)
// On Linux ThemeServiceAuraLinux derives from BraveThemeService instead.
#define ThemeService BraveThemeService
#endif

#define BRAVE_THEMESERVICEFACTORY_BUILDSERVICEINSTANCEFOR \
  GetBraveThemeHelper(static_cast<Profile*>(profile))

// We don't want the original GetThemeHelper to create a NoDestruct static
// helper, so fool it into returning a ref to a deleted object.
#define BRAVE_GETTHEME_HELPER           \
  ThemeHelper* tmp = new ThemeHelper(); \
  const ThemeHelper& dummy = *tmp;      \
  delete tmp;                           \
  return dummy;

#include "../../../../../chrome/browser/themes/theme_service_factory.cc"
#undef BRAVE_GETTHEME_HELPER
#undef BRAVE_THEMESERVICEFACTORY_BUILDSERVICEINSTANCEFOR
#if !defined(OS_LINUX)
#undef ThemeService
#endif
