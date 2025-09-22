/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_utils.h"

#include "brave/browser/ui/bookmark/bookmark_helper.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/grit/theme_resources.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_utils.h"

namespace chrome {

void ToggleBookmarkBarWhenVisible_ChromiumImpl(
    content::BrowserContext* browser_context);

void BraveToggleBookmarkBarState(content::BrowserContext* browser_context) {
  ToggleBookmarkBarWhenVisible_ChromiumImpl(browser_context);
  auto* prefs = user_prefs::UserPrefs::Get(browser_context);
  // On macOS with the View menu or via hotkeys, the options Always show
  // bookmarks is a checkbox. We will keep that checkbox to be Always and Never.
  const bool always_show =
      prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar);
  brave::SetBookmarkState(always_show ? brave::BookmarkBarState::kAlways
                                      : brave::BookmarkBarState::kNever,
                          prefs);
}

}  // namespace chrome

#define IsAppsShortcutEnabled IsAppsShortcutEnabled_Unused
#define ShouldShowAppsShortcutInBookmarkBar \
  ShouldShowAppsShortcutInBookmarkBar_Unused

#define ToggleBookmarkBarWhenVisible                                       \
  ToggleBookmarkBarWhenVisible(content::BrowserContext* browser_context) { \
    BraveToggleBookmarkBarState(browser_context);                          \
  }                                                                        \
  void ToggleBookmarkBarWhenVisible_ChromiumImpl

#if defined(TOOLKIT_VIEWS)
// Override folder icon to use a fixed 20px size similar to saved groups.
#define GetBookmarkFolderIcon GetBookmarkFolderIcon_ChromiumImpl
#endif  // defined(TOOLKIT_VIEWS)

#include <chrome/browser/ui/bookmarks/bookmark_utils.cc>

#if defined(TOOLKIT_VIEWS)
#undef GetBookmarkFolderIcon
#endif  // defined(TOOLKIT_VIEWS)

#undef ToggleBookmarkBarWhenVisible
#undef IsAppsShortcutEnabled
#undef ShouldShowAppsShortcutInBookmarkBar

namespace chrome {

bool IsAppsShortcutEnabled(Profile* profile) {
  return false;
}

bool ShouldShowAppsShortcutInBookmarkBar(Profile* profile) {
  return false;
}

#if defined(TOOLKIT_VIEWS)
// Brave override: return folder icons at 20px like the saved groups icon.
ui::ImageModel GetBookmarkFolderIcon(BookmarkFolderIconType icon_type,
                                     ui::ColorVariant color) {
  const gfx::VectorIcon* id = icon_type == BookmarkFolderIconType::kManaged
                                  ? &vector_icons::kFolderManagedRefreshIcon
                                  : &vector_icons::kFolderChromeRefreshIcon;
  // Use toolbar icon color for visual consistency with other toolbar icons.
  return ui::ImageModel::FromVectorIcon(*id, kColorToolbarButtonIcon, 20);
}
#endif  // defined(TOOLKIT_VIEWS)

}  // namespace chrome
