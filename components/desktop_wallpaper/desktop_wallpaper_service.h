#ifndef BRAVE_COMPONENTS_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_SERVICE_H_
#define BRAVE_COMPONENTS_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_SERVICE_H_

#include "base/files/file_path.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace desktop_wallpaper {
enum class Scaling { kFitToScreen, kFillScreen, kStretchToFill, kCenter };

inline std::optional<Scaling> ScalingFromString(const std::string& scaling) {
  if (scaling == "fitToScreen") {
    return Scaling::kFitToScreen;
  }else if (scaling == "fillScreen") {
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
  static void SetImageAsDesktopWallpaper(
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
      const GURL& url,
      Scaling scaling);

 private:
  static void SetWallpaper(base::FilePath path);
  static void DownloadAndSaveWallpaper(
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
      const GURL& url,
      const base::FilePath& path);
  static net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag();
  static void OnWallpaperDownloaded(base::FilePath path);
};
}  // namespace desktop_wallpaper

#endif
