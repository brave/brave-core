/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_browser_main_extra_parts_views_linux.h"

#include "base/bind.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/libgtkui/gtk_ui.h"
#include "chrome/browser/ui/views/theme_profile_key.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/pref_names.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/native_theme/native_theme_dark_aura.h"

namespace {

ui::NativeTheme* GetNativeThemeForWindow(aura::Window* window) {
  if (!window)
    return nullptr;

  Profile* profile = GetThemeProfileForWindow(window);

  if (!profile) {
    return nullptr;
  }

  // If using the system (GTK) theme, don't use an Aura NativeTheme at all.
  // Instead, CustomThemeSupplier is used.
  if (profile->GetPrefs()->GetBoolean(prefs::kUsesSystemTheme)) {
    return nullptr;
  }

  const bool dark_mode = (profile->IsIncognitoProfile() ||
                          brave::IsTorProfile(profile) ||
                          brave::IsGuestProfile(profile));
  if (dark_mode && BrowserView::GetBrowserViewForNativeWindow(window)) {
    return ui::NativeThemeDarkAura::instance();
  }

  return ui::NativeTheme::GetInstanceForNativeUi();
}

}  // namespace

void BraveBrowserMainExtraPartsViewsLinux::PreEarlyInitialization() {
  views::LinuxUI* gtk_ui = BuildGtkUi();
  gtk_ui->SetNativeThemeOverride(base::Bind(&GetNativeThemeForWindow));
  views::LinuxUI::SetInstance(gtk_ui);
}
