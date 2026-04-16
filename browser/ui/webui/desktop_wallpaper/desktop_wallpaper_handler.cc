#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper_handler.h"

#include "brave/components/desktop_wallpaper/desktop_wallpaper_service.h"

DesktopWallpaperHandler::DesktopWallpaperHandler(
    mojo::PendingReceiver<desktop_wallpaper::mojom::PageHandler> receiver,
    mojo::PendingRemote<desktop_wallpaper::mojom::Page> page,
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      loader_factory_(loader_factory) {}

DesktopWallpaperHandler::~DesktopWallpaperHandler() = default;

void DesktopWallpaperHandler::SetImageAsDesktopWallpaper(const GURL& url, desktop_wallpaper::mojom::Scaling scaling) {
  desktop_wallpaper::DesktopWallpaper::SetImageAsDesktopWallpaper(
      loader_factory_, url, static_cast<desktop_wallpaper::Scaling>(scaling));
}

// void DesktopWallpaperHandler::HandleCloseModal() {}
