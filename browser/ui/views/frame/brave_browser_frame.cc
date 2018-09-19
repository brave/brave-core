/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame.h"

#include "brave/browser/themes/brave_theme_service.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/native_theme/native_theme_dark_aura.h"
#include "ui/views/widget/native_widget.h"

BraveBrowserFrame::BraveBrowserFrame(BrowserView* browser_view)
    : BrowserFrame(browser_view),
      browser_view_(browser_view) {
}

BraveBrowserFrame::~BraveBrowserFrame() {
}

const ui::NativeTheme* BraveBrowserFrame::GetNativeTheme() const {
  // Gets the platform-specific override for NativeTheme,
  // unless we're in dark mode in which case get cross-platform
  // dark theme.
  // TODO: Have platform-specific version of dark theme too.
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_CHROMEOS)
  BraveThemeType active_builtin_theme =
            BraveThemeService::GetActiveBraveThemeType(
                              browser_view_->browser()->profile());
  if (active_builtin_theme == BraveThemeType::BRAVE_THEME_TYPE_DARK ||
      browser_view_->browser()->profile()->GetProfileType() ==
          Profile::INCOGNITO_PROFILE) {
    return ui::NativeThemeDarkAura::instance();
  }
#endif
  // Each platform will implement ui::NativeTheme::GetInstanceForNativeUi
  // separately, which Widget::GetNativeTheme calls.
  return views::Widget::GetNativeTheme();
}