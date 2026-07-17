/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_site_image_source.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/thread_pool.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_source_util.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/mime_util.h"
#include "url/gurl.h"

namespace ntp_background_images {

NTPSponsoredSiteImageSource::NTPSponsoredSiteImageSource(
    NTPBackgroundImagesService* background_images_service)
    : background_images_service_(background_images_service) {}

NTPSponsoredSiteImageSource::~NTPSponsoredSiteImageSource() = default;

std::string NTPSponsoredSiteImageSource::GetSource() {
  return kSponsoredSiteImageHost;
}

void NTPSponsoredSiteImageSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& /*wc_getter*/,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!background_images_service_) {
    return DenyAccess(std::move(callback));
  }

  const base::FilePath request_path =
      base::FilePath::FromUTF8Unsafe(URLToRequestPath(url));
  const std::optional<base::FilePath> file_path =
      background_images_service_->GetSponsoredSiteImageFilePath(request_path);
  if (!file_path) {
    return DenyAccess(std::move(callback));
  }

  AllowAccess(*file_path, std::move(callback));
}

std::string NTPSponsoredSiteImageSource::GetMimeType(const GURL& url) {
  std::string mime_type;
  const base::FilePath file_path = base::FilePath::FromUTF8Unsafe(url.path());
  if (!file_path.empty()) {
    net::GetWellKnownMimeTypeFromFile(file_path, &mime_type);
  }

  return mime_type;
}

bool NTPSponsoredSiteImageSource::AllowCaching() {
  return false;
}

void NTPSponsoredSiteImageSource::ReadFileCallback(
    GotDataCallback callback,
    std::optional<std::string> input) {
  if (!input) {
    return std::move(callback).Run(scoped_refptr<base::RefCountedMemory>());
  }

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

void NTPSponsoredSiteImageSource::AllowAccess(const base::FilePath& file_path,
                                              GotDataCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, file_path),
      base::BindOnce(&NTPSponsoredSiteImageSource::ReadFileCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPSponsoredSiteImageSource::DenyAccess(GotDataCallback callback) {
  std::move(callback).Run(scoped_refptr<base::RefCountedMemory>());
}

}  // namespace ntp_background_images
