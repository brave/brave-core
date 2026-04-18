// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/desktop_wallpaper/desktop_wallpaper.mojom-forward.h"
#include "brave/components/desktop_wallpaper/desktop_wallpaper.mojom.h"
#include "brave/components/desktop_wallpaper/desktop_wallpaper_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/display/screen.h"
#include "url/gurl.h"

DesktopWallpaperHandler::DesktopWallpaperHandler(
    mojo::PendingReceiver<desktop_wallpaper::mojom::PageHandler> receiver,
    mojo::PendingRemote<desktop_wallpaper::mojom::Page> page,
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      loader_factory_(loader_factory) {}

DesktopWallpaperHandler::~DesktopWallpaperHandler() = default;

void DesktopWallpaperHandler::SetImageAsDesktopWallpaper(
    const std::string& path,
    std::vector<desktop_wallpaper::mojom::DisplayInfosPtr> displays,
    desktop_wallpaper::mojom::Scaling scaling) {
  auto status = desktop_wallpaper::DesktopWallpaper::SetImageAsDesktopWallpaper(
      path, std::move(displays),
      static_cast<desktop_wallpaper::Scaling>(scaling));
  page_->ReceiveWallpaperStatus(status);
}

void DesktopWallpaperHandler::FetchImage(const std::string& source) {
  auto req = std::make_unique<network::ResourceRequest>();
  req->url = GURL(source);

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(req),
      DesktopWallpaperHandler::GetNetworkTrafficAnnotationTag());

  url_loader_->SetAllowHttpErrorResults(false);
  url_loader_->DownloadToTempFile(
      loader_factory_.get(),
      base::BindOnce(&DesktopWallpaperHandler::OnFetchImageComplete,
                     weak_ptr_.GetWeakPtr()));

  VLOG(1) << "FetchImage: " << source;
}

void DesktopWallpaperHandler::GetDisplayInfos() {
  auto displays = display::Screen::Get()->GetAllDisplays();
  std::vector<desktop_wallpaper::mojom::DisplayInfosPtr> displays_vec;

  for (const auto& display : displays) {
    auto id = display.id();
    auto label = display.label();
    auto size = display.GetSizeInPixel();

    auto dis = desktop_wallpaper::mojom::DisplayInfos::New();
    dis->id = base::NumberToString(id);
    dis->label = label;
    dis->width = base::NumberToString(size.width());
    dis->height = base::NumberToString(size.height());

    displays_vec.push_back(std::move(dis));
  }

  page_->ReceiveDisplayInfos(std::move(displays_vec));
}

void DesktopWallpaperHandler::OnFetchImageComplete(base::FilePath path) {
  url_loader_.reset();
  if (path.empty()) {
    LOG(ERROR) << "Cannot read from image";
    return;
  }

  base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})
      ->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(
              [](base::FilePath bind_path)
                  -> DesktopWallpaperHandler::FileResult {
                std::string data;
                base::ReadFileToString(bind_path, &data);
                std::string ext = bind_path.Extension();
                if (!ext.empty() && ext[0] == '.') {
                  ext = ext.substr(1);
                }

                return {std::move(data), ext, bind_path.value().c_str()};
              },
              std::move(path)),
          base::BindOnce(&DesktopWallpaperHandler::OnProcessTmpImageComplete,
                         weak_ptr_.GetWeakPtr()));
}

void DesktopWallpaperHandler::OnProcessTmpImageComplete(
    DesktopWallpaperHandler::FileResult result) {
  auto [data, ext, path] = result;

  if (data.empty()) {
    LOG(ERROR) << "Cannot set wallpaper from empty data";

    return;
  }

  page_->ReceiveImage(base::Base64Encode(data), ext, path);
}

net::NetworkTrafficAnnotationTag
DesktopWallpaperHandler::GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("desktop_wallpaper", R"(
    semantic {
      sender: "Desktop Wallpaper Service"
      description: "Fetch a wallpaper from a given image"
      trigger: "User tries to fetch a wallpaper from a given source"
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
