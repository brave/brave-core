/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_dark_aura.h"

void BraveBrowserView::SetStarredState(bool is_starred) {
  BookmarkButton* button = ((BraveToolbarView *)toolbar())->bookmark_button();
  if (button)
    button->SetToggled(is_starred);
}

void BraveBrowserView::OnThemeChanged() {
  // When the theme changes, the native theme may also change (the usage
  // of dark or normal hinges on the browser theme), so we have to
  // propagate both kinds of change.
  // First, make sure we do not propagate ThemeChanged to all children again in
  // response to the native theme change.
  base::AutoReset<bool> reset(&handling_theme_changed_, true);
  // Notify dark (cross-platform) and light (platform-specific) variants
  ui::NativeThemeDarkAura::instance()->NotifyObservers();
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyObservers();

  views::View::OnThemeChanged();
}
