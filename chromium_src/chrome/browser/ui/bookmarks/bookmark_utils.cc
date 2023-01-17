/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "brave/browser/ui/bookmark/bookmark_helper.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/grit/theme_resources.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
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

#if defined(TOOLKIT_VIEWS)
#define GetBookmarkFolderIcon GetBookmarkFolderIcon_UnUsed
#endif

#define ToggleBookmarkBarWhenVisible                                       \
  ToggleBookmarkBarWhenVisible(content::BrowserContext* browser_context) { \
    BraveToggleBookmarkBarState(browser_context);                          \
  }                                                                        \
  void ToggleBookmarkBarWhenVisible_ChromiumImpl

#include "src/chrome/browser/ui/bookmarks/bookmark_utils.cc"
#undef ToggleBookmarkBarWhenVisible
#undef IsAppsShortcutEnabled
#undef ShouldShowAppsShortcutInBookmarkBar

#if defined(TOOLKIT_VIEWS)
#undef GetBookmarkFolderIcon
#endif

namespace chrome {

bool IsAppsShortcutEnabled(Profile* profile) {
  return false;
}

bool ShouldShowAppsShortcutInBookmarkBar(Profile* profile) {
  return false;
}

#if defined(TOOLKIT_VIEWS)
ui::ImageModel GetBookmarkFolderIcon(BookmarkFolderIconType icon_type,
                                     absl::variant<int, SkColor> color) {
  int default_id =
#if BUILDFLAG(IS_WIN)
      IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_WIN_LIGHT;
#elif BUILDFLAG(IS_LINUX)
      IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_LIN_LIGHT;
#else
      IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_LIGHT;
#endif

  const auto generator = [](int default_id, BookmarkFolderIconType icon_type,
                            absl::variant<int, SkColor> color,
                            const ui::ColorProvider* color_provider) {
    gfx::ImageSkia folder;
    SkColor sk_color;
    if (absl::holds_alternative<SkColor>(color)) {
      sk_color = absl::get<SkColor>(color);
    } else {
      DCHECK(color_provider);
      sk_color = color_provider->GetColor(absl::get<ui::ColorId>(color));
    }

    const int resource_id = color_utils::IsDark(sk_color)
#if BUILDFLAG(IS_WIN)
                                ? IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_WIN_LIGHT
                                : IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_WIN_DARK;
#elif BUILDFLAG(IS_LINUX)
                                ? IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_LIN_LIGHT
                                : IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_LIN_DARK;
#else
                                ? IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_LIGHT
                                : IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_DARK;
#endif
    folder = *ui::ResourceBundle::GetSharedInstance()
                  .GetNativeImageNamed(resource_id)
                  .ToImageSkia();
    return gfx::ImageSkia(std::make_unique<RTLFlipSource>(folder),
                          folder.size());
  };
  const gfx::Size size =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(default_id).Size();
  return ui::ImageModel::FromImageGenerator(
      base::BindRepeating(generator, default_id, icon_type, std::move(color)),
      size);
}
#endif

}  // namespace chrome
