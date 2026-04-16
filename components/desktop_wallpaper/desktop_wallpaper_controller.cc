#include <memory>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "desktop_wallpaper_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace desktop_wallpaper {
std::unique_ptr<network::SimpleURLLoader>& GetLoader() {
  static base::NoDestructor<std::unique_ptr<network::SimpleURLLoader>> loader;
  return *loader;
}

void DesktopWallpaper::SetImageAsDesktopWallpaper(
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
    const GURL& url,
    Scaling scaling) {
  base::FilePath home_path;

  CHECK(base::PathService::Get(base::DIR_HOME, &home_path));

  base::FilePath img_name = base::FilePath(url.path()).BaseName();
  base::FilePath path = home_path.Append(img_name);

  DownloadAndSaveWallpaper(loader_factory, url, path);
}

void DesktopWallpaper::DownloadAndSaveWallpaper(
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
    const GURL& url,
    const base::FilePath& path) {
  auto req = std::make_unique<network::ResourceRequest>();
  req->url = url;

  GetLoader() = network::SimpleURLLoader::Create(
      std::move(req), GetNetworkTrafficAnnotationTag());
  GetLoader()->SetAllowHttpErrorResults(false);
  GetLoader()->DownloadToFile(
      loader_factory.get(),
      base::BindOnce(&DesktopWallpaper::OnWallpaperDownloaded), path);
}

void DesktopWallpaper::OnWallpaperDownloaded(base::FilePath path) {
  if (!path.empty()) {
    DesktopWallpaper::SetWallpaper(path);
  }

  GetLoader().reset();
}

net::NetworkTrafficAnnotationTag
DesktopWallpaper::GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("desktop_wallpaper", R"(
    semantic {
      sender: "Desktop Wallpaper Service"
      description: "Set a wallpaper from a given image"
      trigger: "User tries to set a desktop wallpaper from the context menu"
      data: "Image URL"
      destination: WEBSITE
    }

    policy {
      cookies_allowed: NO
      settings: "This feature cannot be disabled by settings"
      policy_exception_justification: "Not implemented"
    }

  )");
}
}  // namespace desktop_wallpaper
