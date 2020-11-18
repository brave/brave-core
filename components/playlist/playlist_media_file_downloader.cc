/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_media_file_downloader.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_types.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace playlist {
namespace {

constexpr unsigned int kRetriesCountOnNetworkChange = 1;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTagForURLLoad() {
  return net::DefineNetworkTrafficAnnotation("playlist_service", R"(
      semantics {
        sender: "Brave Playlist Service"
        description:
          "Fetching media file for newly created playlist"
        trigger:
          "User-initiated for creating new playlist "
        data:
          "media file for playlist"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
      })");
}

base::FilePath::StringType GetFileNameStringFromIndex(int index) {
#if defined(OS_WIN)
  return base::NumberToString16(index);
#else
  return base::NumberToString(index);
#endif
}

// Return false when source file is not appended properly.
bool AppendToFileThenDeleteSource(const base::FilePath& source_path,
                                  const base::FilePath& destination_file_path,
                                  char* read_buffer,
                                  size_t read_buffer_size) {
  base::ScopedFILE source_file(base::OpenFile(source_path, "rb"));
  if (!source_file)
    return false;

  // Read |source_path|'s contents in chunks of read_buffer_size and append
  // to |destination_file_path|.
  size_t num_bytes_read = 0;
  bool got_error = false;
  while ((num_bytes_read =
              fread(read_buffer, 1, read_buffer_size, source_file.get())) > 0) {
    if (!base::AppendToFile(destination_file_path, read_buffer,
                            num_bytes_read)) {
      VLOG(2) << __func__ << " failed to append: " << source_path;
      got_error = true;
      break;
    }
  }

  // Now that it has been copied, delete the source file.
  source_file.reset();
  base::DeleteFile(source_path);
  return !got_error;
}

bool DoGenerateSingleMediaFile(
    const base::FilePath& playlist_dir_path,
    const base::FilePath::StringType& source_media_files_dir,
    const base::FilePath::StringType& unified_media_file_name,
    int num_source_files) {
  const base::FilePath source_files_dir =
      playlist_dir_path.Append(base::FilePath::StringType(
          source_media_files_dir.begin(), source_media_files_dir.end()));

  const base::FilePath unified_media_file_path =
      playlist_dir_path.Append(base::FilePath::StringType(
          unified_media_file_name.begin(), unified_media_file_name.end()));

  // Prepare empty target file to append.
  base::DeleteFile(unified_media_file_path);
  base::File unified_media_file(
      unified_media_file_path,
      base::File::FLAG_CREATE | base::File::FLAG_WRITE);
  if (!unified_media_file.IsValid())
    return false;
  unified_media_file.Close();

  const size_t kReadBufferSize = 1 << 16;  // 64KiB
  char read_buffer[kReadBufferSize];
  bool failed = false;
  for (int i = 0; i < num_source_files; ++i) {
    const base::FilePath media_file_source_path =
        source_files_dir.Append(GetFileNameStringFromIndex(i));
    if (!base::PathExists(media_file_source_path)) {
      VLOG(2) << __func__
              << " file not existed: " << media_file_source_path.value();
      failed = true;
      break;
    }

    if (!AppendToFileThenDeleteSource(media_file_source_path,
                                      unified_media_file_path, read_buffer,
                                      kReadBufferSize)) {
      failed = true;
      break;
    }
  }

  // Delete source files dir.
  base::DeletePathRecursively(source_files_dir);

  if (failed) {
    base::DeleteFile(unified_media_file_path);
    return false;
  }

  return true;
}

}  // namespace

PlaylistMediaFileDownloader::PlaylistMediaFileDownloader(
    Delegate* delegate,
    content::BrowserContext* context,
    base::FilePath::StringType source_media_files_dir,
    base::FilePath::StringType unified_media_file_name,
    std::string media_file_path_key,
    std::string create_params_path_key)
    : delegate_(delegate),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context)
              ->GetURLLoaderFactoryForBrowserProcess()),
      source_media_files_dir_(source_media_files_dir),
      unified_media_file_name_(unified_media_file_name),
      media_file_path_key_(media_file_path_key),
      create_params_path_key_(create_params_path_key),
      weak_factory_(this) {}

PlaylistMediaFileDownloader::~PlaylistMediaFileDownloader() = default;

void PlaylistMediaFileDownloader::NotifyFail(const std::string& id) {
  ResetDownloadStatus();
  delegate_->OnMediaFileGenerationFailed(id);
}

void PlaylistMediaFileDownloader::NotifySucceed(
    const std::string& id,
    const std::string& media_file_path_key,
    const std::string& media_file_path) {
  ResetDownloadStatus();
  delegate_->OnMediaFileReady(id, media_file_path_key, media_file_path);
}

void PlaylistMediaFileDownloader::GenerateSingleMediaFile(
    base::Value&& playlist_value,
    const base::FilePath& base_dir) {
  DCHECK(!in_progress_);

  ResetDownloadStatus();

  in_progress_ = true;
  current_playlist_ = std::move(playlist_value);

  auto* id = current_playlist_.FindStringKey(kPlaylistIDKey);
  DCHECK(id);
  current_playlist_id_ = *id;

  remained_download_files_ = GetNumberOfMediaFileSources();
  media_file_source_files_count_ = remained_download_files_;
  if (media_file_source_files_count_ == 0) {
    VLOG(2) << __func__ << ": Empty media file source list";
    // Consider this as normal if youtubedown.js gives empty source list.
    // Maybe this playlist only has audio or video. not both.
    NotifySucceed(current_playlist_id_, media_file_path_key_, "");
    return;
  }

  playlist_dir_path_ = base_dir.AppendASCII(current_playlist_id_);

  // Create PROFILE_DIR/playlist/ID/source_files dir to store each media files.
  // Then, downloads them in that directory.
  CreateSourceFilesDirThenDownloads();
}

void PlaylistMediaFileDownloader::CreateSourceFilesDirThenDownloads() {
  const base::FilePath source_files_dir =
      playlist_dir_path_.Append(source_media_files_dir_);
  base::PostTaskAndReplyWithResult(
      task_runner(), FROM_HERE,
      base::BindOnce(&base::CreateDirectory, source_files_dir),
      base::BindOnce(&PlaylistMediaFileDownloader::OnSourceFilesDirCreated,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistMediaFileDownloader::OnSourceFilesDirCreated(bool success) {
  if (!success) {
    NotifyFail(current_playlist_id_);
    return;
  }

  DownloadAllMediaFileSources();
}

int PlaylistMediaFileDownloader::GetNumberOfMediaFileSources() {
  DCHECK(in_progress_);
  base::Value* media_files =
      current_playlist_.FindPath(create_params_path_key_);
  return media_files->GetList().size();
}

void PlaylistMediaFileDownloader::DownloadAllMediaFileSources() {
  base::Value* media_files =
      current_playlist_.FindPath(create_params_path_key_);
  for (int i = 0; i < media_file_source_files_count_; ++i) {
    const std::string* url =
        media_files->GetList()[i].FindStringKey(kPlaylistMediaFileUrlKey);
    if (url) {
      DownloadMediaFile(GURL(*url), i);
    } else {
      NOTREACHED() << "Playlist has empty media file url";
      NotifyFail(current_playlist_id_);
      break;
    }
  }
}

void PlaylistMediaFileDownloader::DownloadMediaFile(const GURL& url,
                                                    int index) {
  VLOG(2) << __func__ << ": " << url.spec() << " at: " << index;

  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(url);
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  auto loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTagForURLLoad());
  loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(loader));

  const base::FilePath file_path =
      playlist_dir_path_.Append(source_media_files_dir_)
          .Append(GetFileNameStringFromIndex(index));
  iter->get()->DownloadToFile(
      url_loader_factory_.get(),
      base::BindOnce(&PlaylistMediaFileDownloader::OnMediaFileDownloaded,
                     base::Unretained(this), std::move(iter), index),
      file_path);
}

void PlaylistMediaFileDownloader::OnMediaFileDownloaded(
    SimpleURLLoaderList::iterator iter,
    int index,
    base::FilePath path) {
  url_loaders_.erase(iter);
  VLOG(2) << __func__ << ": downloaded media file at " << path;

  if (path.empty()) {
    // This fail is handled during the generation.
    // See |has_skipped_source_files| in DoGenerateSingleMediaFile().
    // |has_skipped_source_files| will be set to true.
    VLOG(1) << __func__ << ": failed to download media file at " << index;
  }

  remained_download_files_--;

  // If all source files are downloaded, unify them into one media file.
  if (IsDownloadFinished())
    StartSingleMediaFileGeneration();
}

void PlaylistMediaFileDownloader::RequestCancelCurrentPlaylistGeneration() {
  ResetDownloadStatus();
}

void PlaylistMediaFileDownloader::StartSingleMediaFileGeneration() {
  base::PostTaskAndReplyWithResult(
      task_runner(), FROM_HERE,
      base::BindOnce(&DoGenerateSingleMediaFile, playlist_dir_path_,
                     source_media_files_dir_, unified_media_file_name_,
                     media_file_source_files_count_),
      base::BindOnce(&PlaylistMediaFileDownloader::OnSingleMediaFileGenerated,
                     weak_factory_.GetWeakPtr(), current_playlist_id_));
}

// If some of source files are not fetched properly, it should be treated as
// fail.
void PlaylistMediaFileDownloader::OnSingleMediaFileGenerated(
    const std::string& id,
    bool success) {
  // If canceled or canceled and new download is started, |id| will be different
  // with |current_playlist_id_|.
  // Just silently end here.
  if (id != current_playlist_id_)
    return;

  if (success) {
    base::FilePath media_file_path =
        playlist_dir_path_.Append(unified_media_file_name_);
    NotifySucceed(id, media_file_path_key_, media_file_path.AsUTF8Unsafe());
  } else {
    NotifyFail(id);
  }
}

base::SequencedTaskRunner* PlaylistMediaFileDownloader::task_runner() {
  if (!task_runner_) {
    task_runner_ = base::CreateSequencedTaskRunner(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return task_runner_.get();
}

void PlaylistMediaFileDownloader::ResetDownloadStatus() {
  in_progress_ = false;
  remained_download_files_ = 0;
  media_file_source_files_count_ = 0;
  current_playlist_id_.clear();
  current_playlist_ = base::Value();
  url_loaders_.clear();
  playlist_dir_path_.clear();
}

bool PlaylistMediaFileDownloader::IsDownloadFinished() {
  return remained_download_files_ == 0;
}

}  // namespace playlist
