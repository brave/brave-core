/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_data_source.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/task_runner_util.h"
#include "brave/components/playlist/playlist_service.h"
#include "url/gurl.h"

namespace playlist {

namespace {

base::Optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return base::Optional<std::string>();
  return contents;
}

}  // namespace

PlaylistDataSource::PlaylistDataSource(PlaylistService* service)
    : service_(service) {}

PlaylistDataSource::~PlaylistDataSource() {}

std::string PlaylistDataSource::GetSource() {
  return "playlist-image";
}

void PlaylistDataSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback got_data_callback) {
  if (!service_) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }
  base::FilePath thumbnail_path;
  if (!service_->GetThumbnailPath(URLDataSource::URLToRequestPath(url),
                                  &thumbnail_path)) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }
  GetThumbnailImageFile(thumbnail_path, std::move(got_data_callback));
}

void PlaylistDataSource::GetThumbnailImageFile(
    const base::FilePath& image_file_path,
    GotDataCallback got_data_callback) {
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&PlaylistDataSource::OnGotThumbnailImageFile,
                     weak_factory_.GetWeakPtr(), std::move(got_data_callback)));
}

void PlaylistDataSource::OnGotThumbnailImageFile(
    GotDataCallback got_data_callback,
    base::Optional<std::string> input) {
  if (!input) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  scoped_refptr<base::RefCountedMemory> bytes;
  bytes = new base::RefCountedBytes(
      reinterpret_cast<const unsigned char*>(input->c_str()), input->length());
  std::move(got_data_callback).Run(std::move(bytes));
}

std::string PlaylistDataSource::GetMimeType(const std::string&) {
  return "image/jpg";
}

bool PlaylistDataSource::AllowCaching() {
  return false;
}

}  // namespace playlist
