/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_browser_main_extra_parts_views_linux.h"

#include "base/bind.h"
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

  // Use the appropriate native theme for the color mode pref
  BraveThemeType active_builtin_theme =
                            BraveThemeService::GetActiveBraveThemeType(profile);
  const bool dark_mode = (
      active_builtin_theme == BraveThemeType::BRAVE_THEME_TYPE_DARK ||
      profile->GetProfileType() == Profile::INCOGNITO_PROFILE ||
      profile->IsTorProfile());
  if (dark_mode &&
      BrowserView::GetBrowserViewForNativeWindow(window)) {
    return ui::NativeThemeDarkAura::instance();
  }

  return ui::NativeTheme::GetInstanceForNativeUi();
}

}

void BraveBrowserMainExtraPartsViewsLinux::PreEarlyInitialization() {
  views::LinuxUI* gtk_ui = BuildGtkUi();
  gtk_ui->SetNativeThemeOverride(base::Bind(&GetNativeThemeForWindow));
  views::LinuxUI::SetInstance(gtk_ui);
}
