// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/ntp_custom_images_source.h"

#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "brave/components/ntp_background_images/browser/brave_ntp_custom_background_service.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace ntp_background_images {

namespace {

std::string ReadFileToString(const base::FilePath& path) {
  std::string contents;
  base::ReadFileToString(path, &contents);
  return contents;
}

}  // namespace

NTPCustomImagesSource::NTPCustomImagesSource(
    BraveNTPCustomBackgroundService* service)
    : service_(service), weak_factory_(this) {
  DCHECK(service_);
}

NTPCustomImagesSource::~NTPCustomImagesSource() = default;

std::string NTPCustomImagesSource::GetSource() {
  return kCustomWallpaperHost;
}

void NTPCustomImagesSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  GetImageFile(service_->GetImageFilePath(url), std::move(callback));
}

std::string NTPCustomImagesSource::GetMimeType(const GURL& url) {
  return "image/jpeg";
}

bool NTPCustomImagesSource::AllowCaching() {
  return false;
}

void NTPCustomImagesSource::GetImageFile(const base::FilePath& image_file_path,
                                         GotDataCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&NTPCustomImagesSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPCustomImagesSource::OnGotImageFile(GotDataCallback callback,
                                           const std::string& input) {
  std::move(callback).Run(new base::RefCountedBytes(base::as_byte_span(input)));
  return;
}

}  // namespace ntp_background_images
