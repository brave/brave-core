/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_data_source.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "base/containers/heap_array.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/browser/mime_util.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/url_data_source.h"
#include "net/base/filename_util.h"
#include "url/gurl.h"

namespace playlist {

namespace {

#define CHECK_CURRENTLY_NOT_ON_UI_THREAD()                                \
  CHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) \
      << "This must be called on a background thread."

constexpr int64_t kMediaChunkSizeInByte = 1024 * 1024 * 10;  // 10MB

scoped_refptr<base::RefCountedMemory> ReadFileToString(
    const base::FilePath& path) {
  CHECK_CURRENTLY_NOT_ON_UI_THREAD();

  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    VLOG(2) << __FUNCTION__ << " Failed to read " << path;
    return nullptr;
  }

  return base::MakeRefCounted<base::RefCountedBytes>(
      reinterpret_cast<const unsigned char*>(contents.c_str()),
      contents.length());
}

content::URLDataSource::RangeDataResult ReadFileRange(
    const base::FilePath& file_path,
    net::HttpByteRange range) {
  CHECK_CURRENTLY_NOT_ON_UI_THREAD();

  base::File file(file_path,
                  base::File::Flags::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    return {};
  }

  // Note that HTTP range's first and last position are inclusive.
  int64_t first_byte_position =
      range.HasFirstBytePosition() ? range.first_byte_position() : 0;
  int64_t last_byte_position = range.HasLastBytePosition()
                                   ? range.last_byte_position()
                                   : std::numeric_limits<uint32_t>::max();
  int64_t read_size = std::min(kMediaChunkSizeInByte,
                               last_byte_position - first_byte_position + 1);

  auto buffer = base::HeapArray<uint8_t>::Uninit(read_size);
  auto read_result = file.Read(first_byte_position, buffer);
  if (!read_result.has_value()) {
    return {};
  }
  read_size = read_result.value();

  content::URLDataSource::RangeDataResult result;
  result.buffer = base::MakeRefCounted<base::RefCountedBytes>(buffer);
  result.file_size = file.GetLength();
  result.range = net::HttpByteRange::Bounded(
      first_byte_position, first_byte_position + read_size - 1);
  auto mime_type = playlist::mime_util::GetMimeTypeForFileExtension(
                       file_path.FinalExtension())
                       .value_or("video/mp4");
  result.mime_type = mime_type;
  return result;
}

}  // namespace

PlaylistDataSource::DataRequest::DataRequest(const GURL& url) {
  const auto full_path = content::URLDataSource::URLToRequestPath(url);
  const auto paths = base::SplitStringPiece(
      full_path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  CHECK_EQ(paths.size(), 2u);

  id = paths.at(0);
  const auto& type_string = paths.at(1);
  if (type_string == "thumbnail") {
    type = kThumbnail;
  } else if (type_string == "media") {
    type = kMedia;
  } else if (type_string == "favicon") {
    type = kFavicon;
  } else {
    NOTREACHED() << "type is not in {thumbnail,media,favicon}/ : "
                 << type_string;
  }
}

PlaylistDataSource::DataRequest::~DataRequest() = default;

PlaylistDataSource::PlaylistDataSource(Profile* profile,
                                       PlaylistService* service)
    : FaviconSource(profile, chrome::FaviconUrlFormat::kFavicon2),
      service_(service) {}

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

  switch (DataRequest data_request(url); data_request.type) {
    case DataRequest::kThumbnail:
      GetThumbnail(data_request, wc_getter, std::move(got_data_callback));
      break;
    case DataRequest::kFavicon:
      GetFavicon(data_request, wc_getter, std::move(got_data_callback));
      break;
    case DataRequest::kMedia:
      NOTREACHED() << "This request should call StartRangeDataRequest()";
      break;
  }
}

void PlaylistDataSource::StartRangeDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    const net::HttpByteRange& range,
    GotRangeDataCallback callback) {
  DataRequest data_request(url);
  CHECK_EQ(data_request.type, DataRequest::kMedia);
  CHECK(range.IsValid());
  GetMediaFile(data_request, wc_getter, range, std::move(callback));
}

void PlaylistDataSource::GetThumbnail(
    const DataRequest& request,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback got_data_callback) {
  base::FilePath thumbnail_path;
  if (!service_->GetThumbnailPath(request.id, &thumbnail_path)) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(),
      base::BindOnce(&ReadFileToString, thumbnail_path),
      std::move(got_data_callback));
}

void PlaylistDataSource::GetMediaFile(
    const DataRequest& request,
    const content::WebContents::Getter& wc_getter,
    const net::HttpByteRange& range,
    GotRangeDataCallback got_data_callback) {
  base::FilePath media_path;
  if (!service_->HasPlaylistItem(request.id)) {
    std::move(got_data_callback).Run({});
    return;
  }

  auto item = service_->GetPlaylistItem(request.id);
  DCHECK(item->cached);
  if (!net::FileURLToFilePath(item->media_path, &media_path)) {
    std::move(got_data_callback).Run({});
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(),
      base::BindOnce(&ReadFileRange, media_path, range),
      std::move(got_data_callback));
}

void PlaylistDataSource::GetFavicon(
    const DataRequest& request,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback got_data_callback) {
  if (!service_->HasPlaylistItem(request.id)) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  auto item = service_->GetPlaylistItem(request.id);
  GURL favicon_url(
      "chrome://favicon2?allowGoogleServerFallback=0&size=32&pageUrl=" +
      base::EscapeUrlEncodedData(item->page_source.spec(),
                                 /*use_plus=*/false));
  FaviconSource::StartDataRequest(favicon_url, wc_getter,
                                  std::move(got_data_callback));
}

std::string PlaylistDataSource::GetMimeType(const GURL& url) {
  switch (DataRequest data_request(url); data_request.type) {
    case DataRequest::kThumbnail:
      return "image/png";
    case DataRequest::kMedia:
      return "video/mp4";  //  Note that this will be fixed up based on the
                           //  actual file extension in WebUIUrlLoader.
    case DataRequest::kFavicon:
      return FaviconSource::GetMimeType(url);
  }

  NOTREACHED_NORETURN();
}

bool PlaylistDataSource::AllowCaching() {
  return false;
}

bool PlaylistDataSource::SupportsRangeRequests(const GURL& url) const {
  return DataRequest(url).type == DataRequest::kMedia;
}

}  // namespace playlist
