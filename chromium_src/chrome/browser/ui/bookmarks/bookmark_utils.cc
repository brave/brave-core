/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_utils.h"

#include "brave/browser/resources/bookmark_icon/grit/bookmark_icon_resources.h"
#include "brave/browser/ui/bookmark/bookmark_helper.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
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

#if defined(TOOLKIT_VIEWS)
#define GetBookmarkFolderIcon GetBookmarkFolderIcon_UnUsed
#endif

#define ToggleBookmarkBarWhenVisible                                       \
  ToggleBookmarkBarWhenVisible(content::BrowserContext* browser_context) { \
    BraveToggleBookmarkBarState(browser_context);                          \
  }                                                                        \
  void ToggleBookmarkBarWhenVisible_ChromiumImpl

#include <chrome/browser/ui/bookmarks/bookmark_utils.cc>

#if defined(TOOLKIT_VIEWS)
#undef GetBookmarkFolderIcon
#endif  // defined(TOOLKIT_VIEWS)

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

ui::ImageModel GetFilledBookmarkFolderIcon(BookmarkFolderIconType icon_type,
                                           ui::ColorVariant color) {
  int default_id = IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_LIGHT;

  const auto generator = [](int default_id, BookmarkFolderIconType icon_type,
                            ui::ColorVariant color,
                            const ui::ColorProvider* color_provider) {
    gfx::ImageSkia folder;
    SkColor sk_color = color.ResolveToSkColor(color_provider);

    const int resource_id = color_utils::IsDark(sk_color)
                                ? IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_LIGHT
                                : IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_DARK;
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

ui::ImageModel GetBookmarkFolderIcon(BookmarkFolderIconType icon_type,
                                     ui::ColorVariant color) {
  // If the flag is enabled, use the old "filled" bookmark icon.
  if (base::FeatureList::IsEnabled(features::kBraveFilledBookmarkFolderIcon)) {
    return GetFilledBookmarkFolderIcon(icon_type, color);
  }

  const gfx::VectorIcon* id = icon_type == BookmarkFolderIconType::kManaged
                                  ? &vector_icons::kFolderManagedRefreshIcon
                                  : &vector_icons::kFolderChromeRefreshIcon;
  // Use toolbar icon color for visual consistency with other toolbar icons.
  return ui::ImageModel::FromVectorIcon(*id, kColorToolbarButtonIcon, 20);
}

#endif  // defined(TOOLKIT_VIEWS)

}  // namespace chrome
