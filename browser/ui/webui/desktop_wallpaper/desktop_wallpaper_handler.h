#ifndef BRAVE_BROWSER_UI_WEBUI_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_HANDLER_H
#define BRAVE_BROWSER_UI_WEBUI_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_HANDLER_H
#include "brave/components/desktop_wallpaper/desktop_wallpaper_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper.mojom.h"

class DesktopWallpaperHandler : public desktop_wallpaper::mojom::PageHandler {
 public:
  explicit DesktopWallpaperHandler(
      mojo::PendingReceiver<desktop_wallpaper::mojom::PageHandler> receiver,
      mojo::PendingRemote<desktop_wallpaper::mojom::Page> page,
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory
  );
  ~DesktopWallpaperHandler() override;

 private:
  void SetImageAsDesktopWallpaper(const GURL& url, desktop_wallpaper::mojom::Scaling scaling) override;
  // void HandleCloseModal();

  mojo::Receiver<desktop_wallpaper::mojom::PageHandler> receiver_;
  mojo::Remote<desktop_wallpaper::mojom::Page> page_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
};
#endif
