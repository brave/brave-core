/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_data_source.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/byte_size.h"
#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/heap_array.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/content/browser/mime_util.h"
#include "brave/components/playlist/content/browser/playlist_service.h"
#include "brave/components/playlist/core/common/features.h"
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

constexpr base::ByteSize kMediaChunkSize = base::MiBU(1);  // 1MB

// `FinalExtension()` keeps the leading dot, but the mime table is keyed
// without it.
std::string GetMimeTypeForPath(const base::FilePath& path) {
  base::FilePath::StringType extension = path.FinalExtension();
  if (extension.starts_with(FILE_PATH_LITERAL("."))) {
    extension.erase(0, 1);
  }

  if (auto mime_type = mime_util::GetMimeTypeForFileExtension(extension)) {
    return *mime_type;
  }

  // Not in the table because it's only ever a piece of a stream, never a file
  // Playlist would save on its own.
  if (extension == FILE_PATH_LITERAL("m4s")) {
    return "video/mp4";
  }

  return "application/octet-stream";
}

class RefCountedMemMap : public base::RefCountedMemory {
 public:
  explicit RefCountedMemMap(const base::FilePath& path) {
    base::File file = base::File(
        path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
    if (!file.IsValid() ||
        file.GetLength() > static_cast<int64_t>(base::MiBU(100).InBytes())) {
      // In order to avoid OOM crash, limits the file size to 100MB.
      return;
    }
    initialized_ = memory_mapped_file_.Initialize(std::move(file));
  }

  bool initialized() const { return initialized_; }

 private:
  ~RefCountedMemMap() override = default;

  // RefCountedMemory:
  base::span<const uint8_t> AsSpan() const LIFETIME_BOUND override {
    return memory_mapped_file_.bytes();
  }

  base::MemoryMappedFile memory_mapped_file_;
  bool initialized_ = false;
};

scoped_refptr<base::RefCountedMemory> ReadMemoryMappedFile(
    const base::FilePath& path) {
  CHECK_CURRENTLY_NOT_ON_UI_THREAD();

  auto mem_mapped_file = base::MakeRefCounted<RefCountedMemMap>(path);
  if (!mem_mapped_file->initialized()) {
    return nullptr;
  }

  return mem_mapped_file;
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
  auto file_length = file.GetLength();
  if (first_byte_position == file_length) {
    // It looks like the media player tries to make sure that it's the end of
    // file by sending the first byte position as the file size.
    content::URLDataSource::RangeDataResult result;
    result.buffer = base::MakeRefCounted<base::RefCountedBytes>();
    result.file_size = 0;
    result.range =
        net::HttpByteRange::Bounded(first_byte_position, first_byte_position);
    auto mime_type = playlist::mime_util::GetMimeTypeForFileExtension(
                         file_path.FinalExtension())
                         .value_or("video/mp4");
    result.mime_type = mime_type;
    return result;
  }

  int64_t last_byte_position =
      range.HasLastBytePosition()
          ? range.last_byte_position()
          : first_byte_position +
                static_cast<int64_t>(kMediaChunkSize.InBytes()) - 1;
  int64_t read_size = std::min(static_cast<int64_t>(kMediaChunkSize.InBytes()),
                               last_byte_position - first_byte_position + 1);
  CHECK_GE(read_size, 0);

  std::vector<unsigned char> buffer(read_size);
  auto read_result = file.Read(first_byte_position, buffer);
  if (!read_result.has_value()) {
    return {};
  }
  read_size = read_result.value();
  buffer.resize(read_size);

  content::URLDataSource::RangeDataResult result;
  result.buffer =
      base::MakeRefCounted<base::RefCountedBytes>(std::move(buffer));
  result.file_size = file_length;
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
  if (paths.size() < 2) {
    LOG(ERROR) << "Invalid playlist data source URL: " << url.spec();
    return;
  }

  id = paths.at(0);
  const auto& type_string = paths.at(1);

  // Locally saved HLS streams: <id>/hls/<file>, where <file> may itself
  // contain slashes because it comes verbatim from the local manifest.
  if (type_string == "hls" &&
      base::FeatureList::IsEnabled(features::kPlaylistServiceV2)) {
    if (paths.size() < 3) {
      LOG(ERROR) << "HLS request is missing a file name: " << url.spec();
      return;
    }

    base::FilePath file;
    for (const auto& component : base::span(paths).subspan(2u)) {
      // The manifest we generate only ever names plain relative files, so
      // anything that could climb out of the item directory is a rejection,
      // not something to normalize.
      if (component == "." || component == ".." ||
          component.find('\\') != std::string_view::npos) {
        LOG(ERROR) << "Rejecting HLS path component: " << url.spec();
        return;
      }
      file = file.Append(base::FilePath::FromASCII(component));
    }

    if (file.ReferencesParent()) {
      LOG(ERROR) << "Rejecting HLS path referencing parent: " << url.spec();
      return;
    }

    type = DataRequest::Type::kHls;
    hls_file = std::move(file);
    return;
  }

  if (paths.size() != 2) {
    LOG(ERROR) << "Invalid playlist data source URL: " << url.spec();
    return;
  }

  if (type_string == "thumbnail") {
    type = DataRequest::Type::kThumbnail;
  } else if (type_string == "media") {
    type = DataRequest::Type::kMedia;
  } else if (type_string == "favicon") {
    type = DataRequest::Type::kFavicon;
  } else {
    type = DataRequest::Type::kNone;
    LOG(ERROR) << "Invalid playlist data source URL: " << url.spec();
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
    case DataRequest::Type::kNone:
      std::move(got_data_callback).Run(nullptr);
      break;
    case DataRequest::Type::kThumbnail:
      GetThumbnail(data_request, wc_getter, std::move(got_data_callback));
      break;
    case DataRequest::Type::kFavicon:
      GetFavicon(data_request, wc_getter, std::move(got_data_callback));
      break;
    case DataRequest::Type::kHls:
      // Only manifests come through here; segments support range requests.
      GetHlsManifest(data_request, std::move(got_data_callback));
      break;
    case DataRequest::Type::kMedia:
      NOTREACHED() << "This request should call StartRangeDataRequest()";
  }
}

void PlaylistDataSource::StartRangeDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    const net::HttpByteRange& range,
    GotRangeDataCallback callback) {
  DataRequest data_request(url);
  if (!range.IsValid()) {
    std::move(callback).Run({});
    return;
  }

  if (data_request.type == DataRequest::Type::kHls) {
    GetHlsSegment(data_request, range, std::move(callback));
    return;
  }

  if (data_request.type != DataRequest::Type::kMedia) {
    std::move(callback).Run({});
    return;
  }
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
      base::BindOnce(&ReadMemoryMappedFile, thumbnail_path),
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

base::FilePath PlaylistDataSource::ResolveHlsPath(
    const DataRequest& request) const {
  if (request.hls_file.empty() || !service_->HasPlaylistItem(request.id)) {
    return base::FilePath();
  }

  // Stream files live in their own subdirectory so they can't collide with
  // the item's `media_file` / `thumbnail`.
  const base::FilePath item_dir =
      service_->GetPlaylistItemDirPath(request.id).AppendASCII("hls");
  const base::FilePath path = item_dir.Append(request.hls_file);

  // `DataRequest` already rejects "..", but the item directory is the security
  // boundary here, so confirm it rather than trusting that.
  if (!item_dir.IsParent(path)) {
    return base::FilePath();
  }

  return path;
}

void PlaylistDataSource::GetHlsManifest(const DataRequest& request,
                                        GotDataCallback got_data_callback) {
  const base::FilePath path = ResolveHlsPath(request);
  if (path.empty()) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(), base::BindOnce(&ReadMemoryMappedFile, path),
      std::move(got_data_callback));
}

void PlaylistDataSource::GetHlsSegment(const DataRequest& request,
                                       const net::HttpByteRange& range,
                                       GotRangeDataCallback got_data_callback) {
  const base::FilePath path = ResolveHlsPath(request);
  if (path.empty()) {
    std::move(got_data_callback).Run({});
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(), base::BindOnce(&ReadFileRange, path, range),
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
  if (url.is_empty()) {
    // This could be reached on start up.
    return {};
  }

  switch (DataRequest data_request(url); data_request.type) {
    case DataRequest::Type::kThumbnail:
      return "image/png";
    case DataRequest::Type::kMedia:
      return "video/mp4";  //  Note that this will be fixed up based on the
                           //  actual file extension in WebUIUrlLoader.
    case DataRequest::Type::kFavicon:
      return FaviconSource::GetMimeType(url);
    case DataRequest::Type::kHls:
      return GetMimeTypeForPath(data_request.hls_file);
    case DataRequest::Type::kNone:
      return {};
  }
}

bool PlaylistDataSource::AllowCaching() {
  return false;
}

bool PlaylistDataSource::SupportsRangeRequests(const GURL& url) const {
  if (url.is_empty()) {
    // This could be reached on start up.
    return false;
  }

  DataRequest data_request(url);
  if (data_request.type == DataRequest::Type::kHls) {
    // Manifests are small and read whole; only segments are worth ranging.
    return !data_request.hls_file.MatchesFinalExtension(
        FILE_PATH_LITERAL(".m3u8"));
  }

  return data_request.type == DataRequest::Type::kMedia;
}

}  // namespace playlist
