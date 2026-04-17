// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper_ui.h"

#include <optional>
#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/resources/desktop_wallpaper/grit/desktop_wallpaper_generated_map.h"
#include "brave/browser/resources/desktop_wallpaper/grit/desktop_wallpaper_static_resources.h"
#include "brave/browser/resources/desktop_wallpaper/grit/desktop_wallpaper_static_resources_map.h"
#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
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
      std::move(receiver), std::move(page), profile->GetURLLoaderFactory());
}

DesktopWallpaperUIConfig::DesktopWallpaperUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kDesktopWallpaperHost) {}

DesktopWallpaperDialogDelegate::DesktopWallpaperDialogDelegate(
    const std::string& image_url,
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
    content::WebContents* initiator_web_contents)
    : content::WebContentsObserver(initiator_web_contents),
      image_url_(image_url),
      loader_factory_(loader_factory) {
  std::optional<std::string> react_props =
      base::WriteJson(base::DictValue().Set("image_url", image_url));

  if (!react_props || react_props->empty()) {
    LOG(ERROR) << "DesktopWallpaperDialogDelegate: cannot get props for the "
                  "react component";
    return;
  }

  set_dialog_content_url(GURL(kDesktopWallpaperURL));
  set_show_dialog_title(false);
  set_can_resize(false);
  set_delete_on_close(false);
  set_dialog_modal_type(ui::mojom::ModalType::kWindow);
  set_dialog_args(*react_props);
}

DesktopWallpaperDialogDelegate::~DesktopWallpaperDialogDelegate() = default;

void DesktopWallpaperDialogDelegate::SetConstrainedDelegate(
    ConstrainedWebDialogDelegate* delegate) {
  constrained_delegate_ = delegate;
}

void DesktopWallpaperDialogDelegate::OnDialogClosed(
    const std::string& json_retval) {
  if (constrained_delegate_) {
    auto released_contents = constrained_delegate_->ReleaseWebContents();
    constrained_delegate_ = nullptr;
  }
  WebDialogDelegate::OnDialogClosed(json_retval);
}

void DesktopWallpaperDialogDelegate::WebContentsDestroyed() {
  if (constrained_delegate_) {
    auto released_contents = constrained_delegate_->ReleaseWebContents();
    constrained_delegate_ = nullptr;
  }
}

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
