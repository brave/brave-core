#ifndef BRAVE_COMPONENTS_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_SERVICE_H_
#define BRAVE_COMPONENTS_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_SERVICE_H_

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper.mojom.h"

namespace desktop_wallpaper {
enum class Scaling { kFitToScreen, kFillScreen, kStretchToFill, kCenter };

inline std::optional<Scaling> ScalingFromString(const std::string& scaling) {
  if (scaling == "fitToScreen") {
    return Scaling::kFitToScreen;
  } else if (scaling == "fillScreen") {
    return Scaling::kFillScreen;
  } else if (scaling == "stretchToFill") {
    return Scaling::kStretchToFill;
  } else if (scaling == "center") {
    return Scaling::kCenter;
  }

  return std::nullopt;
}

class DesktopWallpaper {
 public:
  static desktop_wallpaper::mojom::WallpaperStatus SetImageAsDesktopWallpaper(
      std::string path,
      std::vector<desktop_wallpaper::mojom::DisplayInfosPtr> displays,
      Scaling scaling);

 private:
  static desktop_wallpaper::mojom::WallpaperStatus SetWallpaper(
      base::FilePath path,
      std::vector<desktop_wallpaper::mojom::DisplayInfosPtr> displays,
      Scaling scaling);
};
}  // namespace desktop_wallpaper

#endif
