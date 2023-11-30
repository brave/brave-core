/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_data_source.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/escape.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "net/base/filename_util.h"
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

PlaylistDataSource::PlaylistDataSource(Profile* profile)
    : FaviconSource(profile, chrome::FaviconUrlFormat::kFavicon2),
      profile_(profile) {}

PlaylistDataSource::~PlaylistDataSource() = default;

std::string PlaylistDataSource::GetSource() {
  return "chrome-untrusted://playlist-data/";
}

void PlaylistDataSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback got_data_callback) {
  PlaylistService* playlist_service =
      PlaylistServiceFactory::GetForBrowserContext(profile_.get());

  if (!playlist_service) {
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
    NOTREACHED()
        << "path is not in expected form: /id/{thumbnail,media,favicon}/ vs "
        << path;
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  base::FilePath data_path;
  if (type_string == "thumbnail") {
    if (!playlist_service->GetThumbnailPath(id, &data_path)) {
      std::move(got_data_callback).Run(nullptr);
      return;
    }
  } else if (type_string == "media") {
    if (!playlist_service->HasPlaylistItem(id)) {
      std::move(got_data_callback).Run(nullptr);
      return;
    }

    auto item = playlist_service->GetPlaylistItem(id);
    DCHECK(item->cached);
    if (!net::FileURLToFilePath(item->media_path, &data_path)) {
      std::move(got_data_callback).Run(nullptr);
    }
  } else if (type_string == "favicon") {
    if (!playlist_service->HasPlaylistItem(id)) {
      std::move(got_data_callback).Run(nullptr);
      return;
    }

    auto item = playlist_service->GetPlaylistItem(id);
    GURL favicon_url(
        "chrome://favicon2?allowGoogleServerFallback=0&size=32&pageUrl=" +
        base::EscapeUrlEncodedData(item->page_source.spec(),
                                   /*use_plus=*/false));
    FaviconSource::StartDataRequest(favicon_url, wc_getter,
                                    std::move(got_data_callback));
    return;
  } else {
    NOTREACHED() << "type is not in {thumbnail,media,favicon}/ : "
                 << type_string;
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
    NOTREACHED()
        << "path is not in expected form: /id/{thumbnail,media,favicon}/ vs "
        << path;
    return std::string();
  }

  if (type_string == "thumbnail") {
    return "image/png";
  }

  // TODO(sko) Decide mime type based on the file extension.
  if (type_string == "media") {
    return "video/mp4";
  }

  if (type_string == "favicon") {
    return FaviconSource::GetMimeType(url);
  }

  NOTREACHED() << "type is neither of {thumbnail,media,favicon}/ : "
               << type_string;
  return std::string();
}

bool PlaylistDataSource::AllowCaching() {
  return false;
}

}  // namespace playlist
