/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_utils.h"

#define IsAppsShortcutEnabled IsAppsShortcutEnabled_Unused
#define ShouldShowAppsShortcutInBookmarkBar \
  ShouldShowAppsShortcutInBookmarkBar_Unused

#if defined(TOOLKIT_VIEWS)
#define GetBookmarkFolderIcon GetBookmarkFolderIcon_UnUsed
#endif

#include "src/chrome/browser/ui/bookmarks/bookmark_utils.cc"

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
