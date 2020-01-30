/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_playlists_source.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/playlists/playlists_service_factory.h"
#include "brave/components/playlists/browser/playlists_controller.h"
#include "brave/components/playlists/browser/playlists_service.h"
#include "chrome/browser/profiles/profile.h"
#include "url/gurl.h"

namespace brave_playlists {

namespace {

void ThumbnailLoaded(content::URLDataSource::GotDataCallback got_data_callback,
                     std::unique_ptr<std::string> thumbnail_data,
                     bool did_load_file) {
  if (thumbnail_data->size() && did_load_file) {
    std::move(got_data_callback)
        .Run(new base::RefCountedBytes(
            reinterpret_cast<const unsigned char*>(thumbnail_data->data()),
            thumbnail_data->size()));
  } else {
    std::move(got_data_callback).Run(nullptr);
  }
}

}  // namespace

BravePlaylistsSource::BravePlaylistsSource(Profile* profile)
    : profile_(profile->GetOriginalProfile()) {
  task_runner_ = base::CreateSequencedTaskRunner(
      {base::ThreadPool(), base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

BravePlaylistsSource::~BravePlaylistsSource() {}

std::string BravePlaylistsSource::GetSource() {
  return "playlists-image";
}

void BravePlaylistsSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    content::URLDataSource::GotDataCallback got_data_callback) {
  PlaylistsService* service = PlaylistsServiceFactory::GetForProfile(profile_);
  if (!service) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }
  PlaylistsController* controller = service->controller();
  if (!controller || !controller->initialized()) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }
  base::FilePath thumbnail_path;
  if (!controller->GetThumbnailPath(url.path(), &thumbnail_path)) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }
  base::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::ThreadPool(), base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&base::PathExists, thumbnail_path),
      base::BindOnce(&BravePlaylistsSource::StartDataRequestAfterPathExists,
                     weak_factory_.GetWeakPtr(), thumbnail_path,
                     std::move(got_data_callback)));
}

void BravePlaylistsSource::StartDataRequestAfterPathExists(
    const base::FilePath& thumbnail_path,
    content::URLDataSource::GotDataCallback got_data_callback,
    bool path_exists) {
  if (!path_exists) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  auto thumbnail_data = std::make_unique<std::string>();
  std::string* data = thumbnail_data.get();
  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::BindOnce(&base::ReadFileToString, thumbnail_path, data),
      base::BindOnce(&ThumbnailLoaded, std::move(got_data_callback),
                     std::move(thumbnail_data)));
}

std::string BravePlaylistsSource::GetMimeType(const std::string&) {
  return "image/jpg";
}

bool BravePlaylistsSource::AllowCaching() {
  return false;
}

bool BravePlaylistsSource::ShouldReplaceExistingSource() {
  return false;
}

bool BravePlaylistsSource::ShouldServiceRequest(
    const GURL& url,
    content::ResourceContext* resource_context,
    int render_process_id) {
  return URLDataSource::ShouldServiceRequest(url, resource_context,
                                             render_process_id);
}

}  // namespace brave_playlists
