// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_HANDLER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/desktop_wallpaper/desktop_wallpaper.mojom-shared.h"
#include "brave/components/desktop_wallpaper/desktop_wallpaper.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

class DesktopWallpaperHandler : public desktop_wallpaper::mojom::PageHandler {
 public:
  explicit DesktopWallpaperHandler(
      mojo::PendingReceiver<desktop_wallpaper::mojom::PageHandler> receiver,
      mojo::PendingRemote<desktop_wallpaper::mojom::Page> page,
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory);
  ~DesktopWallpaperHandler() override;

 private:
  struct FileResult {
    std::string data;
    std::string ext;
    std::string path;
  };

  void SetImageAsDesktopWallpaper(
      const std::string& path,
      std::vector<desktop_wallpaper::mojom::DisplayInfosPtr> displays,
      desktop_wallpaper::mojom::Scaling scaling) override;
  void FetchImage(const std::string& source) override;
  void GetDisplayInfos() override;
  void OnFetchImageComplete(base::FilePath path);
  void OnProcessTmpImageComplete(FileResult result);

  net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag();

  mojo::Receiver<desktop_wallpaper::mojom::PageHandler> receiver_;
  mojo::Remote<desktop_wallpaper::mojom::Page> page_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  base::WeakPtrFactory<DesktopWallpaperHandler> weak_ptr_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_DESKTOP_WALLPAPER_DESKTOP_WALLPAPER_HANDLER_H_
