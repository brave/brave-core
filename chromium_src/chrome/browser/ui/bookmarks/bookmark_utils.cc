/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_utils.h"

#define IsAppsShortcutEnabled IsAppsShortcutEnabled_Unused
#define ShouldShowAppsShortcutInBookmarkBar \
  ShouldShowAppsShortcutInBookmarkBar_Unused

#if defined(TOOLKIT_VIEWS)
#define GetBookmarkFolderIcon GetBookmarkFolderIcon_UnUsed
#endif

#include "../../../../../../chrome/browser/ui/bookmarks/bookmark_utils.cc"

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
  int default_id = IDR_BRAVE_BOOKMARK_FOLDER_CLOSED;
  const auto generator = [](int default_id, BookmarkFolderIconType icon_type,
                            absl::variant<int, SkColor> color,
                            const ui::NativeTheme* native_theme) {
    gfx::ImageSkia folder;
    SkColor sk_color;
    if (absl::holds_alternative<SkColor>(color)) {
      sk_color = absl::get<SkColor>(color);
    } else {
      DCHECK(native_theme);
      sk_color = native_theme->GetSystemColor(
          static_cast<ui::NativeTheme::ColorId>(absl::get<int>(color)));
    }

    const int resource_id = color_utils::IsDark(sk_color)
                                ? IDR_BRAVE_BOOKMARK_FOLDER_CLOSED
                                : IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_WHITE;
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
