#ifndef BRAVE_BROWSER_UI_WEBUI_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_UI_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "ui/webui/mojo_web_ui_controller.h"

class DesktopWallpaperHandler;

class DesktopWallpaperUI : public ConstrainedWebDialogUI,
                           public ui::EnableMojoWebUI,
                           public desktop_wallpaper::mojom::PageHandlerFactory {
 public:
  explicit DesktopWallpaperUI(content::WebUI* web_ui);
  ~DesktopWallpaperUI() override;
  DesktopWallpaperUI(const DesktopWallpaperUI&) = delete;
  DesktopWallpaperUI& operator=(const DesktopWallpaperUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<desktop_wallpaper::mojom::PageHandlerFactory>
          receiver);

 private:
  // desktop_wallpaper::mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<desktop_wallpaper::mojom::Page> page,
      mojo::PendingReceiver<desktop_wallpaper::mojom::PageHandler> receiver)
      override;

  std::unique_ptr<DesktopWallpaperHandler> page_handler_;
  mojo::Receiver<desktop_wallpaper::mojom::PageHandlerFactory>
      page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class DesktopWallpaperUIConfig
    : public content::DefaultWebUIConfig<DesktopWallpaperUI> {
 public:
  DesktopWallpaperUIConfig();
};

class DesktopWallpaperDialogDelegate : public ui::WebDialogDelegate,
                                     public content::WebContentsObserver {
 public:
  DesktopWallpaperDialogDelegate(
      const std::string& image_url,
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
      content::WebContents* initiator_web_contents);
  ~DesktopWallpaperDialogDelegate() override;

  void SetConstrainedDelegate(ConstrainedWebDialogDelegate* delegate);

 private:
  GURL GetDialogContentURL() const override;
  void GetDialogSize(gfx::Size* size) const override;
  void GetMinimumDialogSize(gfx::Size* size) const override;
  void OnDialogClosed(const std::string& json_retval) override;

  void WebContentsDestroyed() override;

  raw_ptr<ConstrainedWebDialogDelegate> constrained_delegate_ = nullptr;
  std::string image_url_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_UI_H_
