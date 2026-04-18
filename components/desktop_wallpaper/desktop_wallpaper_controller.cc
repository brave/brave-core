// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <filesystem>
#include <vector>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "brave/components/desktop_wallpaper/desktop_wallpaper.mojom.h"
#include "chrome/common/chrome_paths.h"
#include "desktop_wallpaper_service.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace desktop_wallpaper {
desktop_wallpaper::mojom::WallpaperStatus
DesktopWallpaper::SetImageAsDesktopWallpaper(
    const std::string path,
    std::vector<desktop_wallpaper::mojom::DisplayInfosPtr> displays,
    Scaling scaling) {
  if (path.empty() || !std::filesystem::exists(path)) {
    LOG(ERROR) << "Cannot set image as desktop wallpaper: path is empty or "
                  "does not exist";

    return desktop_wallpaper::mojom::WallpaperStatus::failure;
  }

  std::filesystem::path p = path;
  auto ext = p.extension().string().substr(
      1);  // don't really need to have the dot of the file extension as well
  auto filename = displays.size() > 1
                      ? ext
                      : absl::StrFormat("%s_%s", ext, displays[0]->id);

  base::FilePath user_path =
      base::PathService::CheckedGet(chrome::DIR_USER_DATA);

  auto wallpaper_dir = user_path.Append("wallpapers");
  auto wallpaper = wallpaper_dir.Append(filename).value();

  std::error_code ec;

  if (!std::filesystem::exists(wallpaper_dir.value(), ec)) {
    std::filesystem::create_directories(wallpaper_dir.value(), ec);

    if (ec) {
      LOG(ERROR) << "Failed to create wallpaper directory: " << ec.message();

      return desktop_wallpaper::mojom::WallpaperStatus::failure;
    }
  }

  auto is_current_screen = [&displays](
                               const std::filesystem::directory_entry& e) {
    // we are setting a wallpaper
    // for every screen, so we allow the for loop to delete every file
    // inside the folder
    if (displays.size() > 1) {
      return true;
    }

    // otherwise we only get the file with the ID of the screen the user
    // wants a new wallpaper on
    auto parts = base::SplitString(e.path().filename().string(), "_",
                                   base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (parts.size() < 2) {
      return false;
    }
    return e.is_regular_file() && parts[1] == displays[0]->id;
  };

  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(wallpaper_dir.value())) {
    if (!is_current_screen(entry)) {
      continue;
    }
    std::filesystem::remove(entry, ec);

    if (ec) {
      LOG(ERROR) << "Failed to remove existing wallpaper: " << ec.message();

      return desktop_wallpaper::mojom::WallpaperStatus::failure;
    }
  }

  std::filesystem::copy(path, wallpaper, ec);

  if (ec) {
    LOG(ERROR) << "Failed to copy wallpaper: " << ec.message();

    return desktop_wallpaper::mojom::WallpaperStatus::failure;
  }

  std::filesystem::remove(path, ec);

  if (ec) {
    LOG(ERROR) << "Failed to remove temp file: " << ec.message();
  }

  return DesktopWallpaper::SetWallpaper(static_cast<base::FilePath>(wallpaper),
                                        std::move(displays), scaling);
}
}  // namespace desktop_wallpaper
