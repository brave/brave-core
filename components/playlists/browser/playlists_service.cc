/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlists/browser/playlists_service.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/common/extensions/api/brave_playlists.h"
#include "brave/components/playlists/browser/playlists_controller.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/event_router.h"

namespace {

const base::FilePath::StringType kBaseDirName(FILE_PATH_LITERAL("playlists"));

}  // namespace

PlaylistsService::PlaylistsService(content::BrowserContext* context)
    : observer_(this),
      base_dir_(context->GetPath().Append(kBaseDirName)),
      context_(context),
      controller_(new PlaylistsController(context)),
      weak_factory_(this) {
  observer_.Add(controller_.get());
}

PlaylistsService::~PlaylistsService() {}

bool PlaylistsService::Init() {
  return base::PostTaskAndReplyWithResult(
      file_task_runner(), FROM_HERE,
      base::BindOnce(&base::CreateDirectory, base_dir_),
      base::BindOnce(&PlaylistsService::OnBaseDirectoryReady,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistsService::OnBaseDirectoryReady(bool ready) {
  // If we can't create directory in context dir, give up.
  if (!ready) {
    OnPlaylistsInitialized(false);
    return;
  }

  controller_->Init(base_dir_);

  // Not used anymore from now.
  file_task_runner_.reset();
}

void PlaylistsService::OnPlaylistsInitialized(bool initialized) {
  auto event = std::make_unique<extensions::Event>(
      extensions::events::BRAVE_PLAYLISTS_ON_INITIALIZED,
      extensions::api::brave_playlists::OnInitialized::kEventName,
      extensions::api::brave_playlists::OnInitialized::Create(initialized),
      context_);

  extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
}

void PlaylistsService::OnPlaylistsChanged(const PlaylistsChangeParams& params) {
  auto event = std::make_unique<extensions::Event>(
      extensions::events::BRAVE_PLAYLISTS_ON_PLAYLISTS_CHANGED,
      extensions::api::brave_playlists::OnPlaylistsChanged::kEventName,
      extensions::api::brave_playlists::OnPlaylistsChanged::Create(
          PlaylistsChangeParams::GetPlaylistsChangeTypeAsString(
              params.change_type),
          params.playlist_id),
      context_);

  extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
}

void PlaylistsService::OnPlaylistsDownloadRequested(const std::string& url) {
  auto event = std::make_unique<extensions::Event>(
      extensions::events::BRAVE_PLAYLISTS_ON_DOWNLOAD_REQUESTED,
      extensions::api::brave_playlists::OnDownloadRequested::kEventName,
      extensions::api::brave_playlists::OnDownloadRequested::Create(url),
      context_);

  extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
}

base::SequencedTaskRunner* PlaylistsService::file_task_runner() {
  if (!file_task_runner_) {
    file_task_runner_ = base::CreateSequencedTaskRunner(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return file_task_runner_.get();
}
