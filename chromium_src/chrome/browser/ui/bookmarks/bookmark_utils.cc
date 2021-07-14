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
ui::ImageModel GetBookmarkFolderIcon(SkColor text_color) {
  int resource_id = color_utils::IsDark(text_color)
                        ? IDR_BRAVE_BOOKMARK_FOLDER_CLOSED
                        : IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_WHITE;
  gfx::ImageSkia folder = *ui::ResourceBundle::GetSharedInstance()
                               .GetNativeImageNamed(resource_id)
                               .ToImageSkia();
  return ui::ImageModel::FromImageSkia(
      gfx::ImageSkia(std::make_unique<RTLFlipSource>(folder), folder.size()));
}
#endif

}  // namespace chrome
