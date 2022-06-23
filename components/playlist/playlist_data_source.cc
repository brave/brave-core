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
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/playlist/playlist_service.h"
#include "url/gurl.h"

namespace playlist {

namespace {

scoped_refptr<base::RefCountedMemory> ReadFileToString(
    const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return nullptr;

  return base::MakeRefCounted<base::RefCountedBytes>(
      reinterpret_cast<const unsigned char*>(contents.c_str()),
      contents.length());
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
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(),
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&PlaylistDataSource::OnGotThumbnailImageFile,
                     weak_factory_.GetWeakPtr(), std::move(got_data_callback)));
}

void PlaylistDataSource::OnGotThumbnailImageFile(
    GotDataCallback got_data_callback,
    scoped_refptr<base::RefCountedMemory> input) {
  std::move(got_data_callback).Run(input);
}

std::string PlaylistDataSource::GetMimeType(const std::string&) {
  return "image/jpg";
}

bool PlaylistDataSource::AllowCaching() {
  return false;
}

}  // namespace playlist
