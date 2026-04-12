#ifndef BRAVE_COMPONENTS_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_SERVICE_H_
#define BRAVE_COMPONENTS_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_SERVICE_H_

#include "base/files/file_path.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace desktop_wallpaper {
class DesktopWallpaper {
 public:
  static void SetImageAsDesktopWallpaper(
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
      const GURL& url);

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
