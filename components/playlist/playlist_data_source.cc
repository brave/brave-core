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
  if (!base::ReadFileToString(path, &contents)) {
    VLOG(2) << __FUNCTION__ << " Failed to read " << path;
    return nullptr;
  }

  return base::MakeRefCounted<base::RefCountedBytes>(
      reinterpret_cast<const unsigned char*>(contents.c_str()),
      contents.length());
}

}  // namespace

PlaylistDataSource::PlaylistDataSource(PlaylistService* service)
    : service_(service) {}

PlaylistDataSource::~PlaylistDataSource() = default;

std::string PlaylistDataSource::GetSource() {
  return "chrome-untrusted://playlist-data/";
}

void PlaylistDataSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback got_data_callback) {
  if (!service_) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }
  std::string path = URLDataSource::URLToRequestPath(url);
  std::string id;
  std::string type_string;
  if (auto pos = path.find("/"); pos != std::string::npos) {
    id = std::string(path.begin(), path.begin() + pos);
    type_string = std::string(path.begin() + pos, path.end());
    type_string.erase(std::remove(type_string.begin(), type_string.end(), '/'),
                      type_string.end());
  } else {
    NOTREACHED() << "path is not in expected form: /id/{thumbnail,media}/ vs "
                 << path;
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  base::FilePath data_path;
  if (type_string == "thumbnail") {
    if (!service_->GetThumbnailPath(id, &data_path)) {
      std::move(got_data_callback).Run(nullptr);
      return;
    }
  } else if (type_string == "media") {
    if (!service_->GetMediaPath(id, &data_path)) {
      std::move(got_data_callback).Run(nullptr);
      return;
    }
  } else {
    NOTREACHED() << "type is neither of {thumbnail,media}/ : " << type_string;
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  GetDataFile(data_path, std::move(got_data_callback));
}

void PlaylistDataSource::GetDataFile(const base::FilePath& data_path,
                                     GotDataCallback got_data_callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(), base::BindOnce(&ReadFileToString, data_path),
      base::BindOnce(&PlaylistDataSource::OnGotDataFile,
                     weak_factory_.GetWeakPtr(), std::move(got_data_callback)));
}

void PlaylistDataSource::OnGotDataFile(
    GotDataCallback got_data_callback,
    scoped_refptr<base::RefCountedMemory> input) {
  std::move(got_data_callback).Run(input);
}

std::string PlaylistDataSource::GetMimeType(const GURL& url) {
  const std::string path = URLDataSource::URLToRequestPath(url);
  std::string id;
  std::string type_string;
  if (auto pos = path.find("/"); pos != std::string::npos) {
    id = std::string(path.begin(), path.begin() + pos);
    type_string = std::string(path.begin() + pos, path.end());
    type_string.erase(std::remove(type_string.begin(), type_string.end(), '/'),
                      type_string.end());
  } else {
    NOTREACHED() << "path is not in expected form: /id/{thumbnail,media}/ vs "
                 << path;
    return std::string();
  }

  if (type_string == "thumbnail")
    return "image/jpeg";

  // TODO(sko) Decide mime type based on the file extension.
  if (type_string == "media")
    return "video/mp4";

  NOTREACHED() << "type is neither of {thumbnail,media}/ : " << type_string;
  return std::string();
}

bool PlaylistDataSource::AllowCaching() {
  return false;
}

}  // namespace playlist
