#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper_ui.h"

#include <utility>

#include "brave/browser/resources/desktop_wallpaper/grit/desktop_wallpaper_generated_map.h"
#include "brave/browser/resources/desktop_wallpaper/grit/desktop_wallpaper_static_resources.h"
#include "brave/browser/resources/desktop_wallpaper/grit/desktop_wallpaper_static_resources_map.h"
#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper_handler.h"
#include "brave/components/desktop_wallpaper/desktop_wallpaper_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

DesktopWallpaperUI::DesktopWallpaperUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui),
      ui::EnableMojoWebUI(web_ui,
                          /*enable_chrome_send=*/true,
                          /*enable_chrome_histograms=*/true) {
  auto* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kDesktopWallpaperHost);

  webui::SetupWebUIDataSource(
      source, kDesktopWallpaperGenerated,
      IDR_DESKTOP_WALLPAPER_STATIC_DESKTOP_WALLPAPER_ROOT_HTML);

  source->AddResourcePaths(kDesktopWallpaperStaticResources);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome://resources chrome://theme chrome://image "
      "chrome://favicon2 chrome://app-icon chrome://extension-icon "
      "chrome://fileicon blob: data: https: 'self';");
}

DesktopWallpaperUI::~DesktopWallpaperUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(DesktopWallpaperUI)

void DesktopWallpaperUI::BindInterface(
    mojo::PendingReceiver<desktop_wallpaper::mojom::PageHandlerFactory>
        receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void DesktopWallpaperUI::CreatePageHandler(
    mojo::PendingRemote<desktop_wallpaper::mojom::Page> page,
    mojo::PendingReceiver<desktop_wallpaper::mojom::PageHandler> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  page_handler_ = std::make_unique<DesktopWallpaperHandler>(
      std::move(receiver), std::move(page),
      profile->GetURLLoaderFactory());
}

DesktopWallpaperUIConfig::DesktopWallpaperUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kDesktopWallpaperHost) {}


DesktopWallpaperDialogDelegate::DesktopWallpaperDialogDelegate(
    const std::string& image_url,
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory)
    : image_url_(image_url), loader_factory_(loader_factory) {

  set_dialog_content_url(GURL(kDesktopWallpaperURL));
  set_show_dialog_title(false);
  set_can_resize(false);
  set_dialog_modal_type(ui::mojom::ModalType::kWindow);
  set_dialog_args("\"" + image_url + "\"");
}

DesktopWallpaperDialogDelegate::~DesktopWallpaperDialogDelegate() = default;

GURL DesktopWallpaperDialogDelegate::GetDialogContentURL() const {
  return GURL(kDesktopWallpaperURL);
}

void DesktopWallpaperDialogDelegate::GetDialogSize(gfx::Size* size) const {
  *size = gfx::Size(400, 630);
}

void DesktopWallpaperDialogDelegate::GetMinimumDialogSize(
    gfx::Size* size) const {
  *size = gfx::Size(400, 630);
}
