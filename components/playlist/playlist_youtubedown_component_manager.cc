/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_youtubedown_component_manager.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "brave/components/playlist/playlist_youtubedown_component_installer.h"

namespace playlist {

namespace {

constexpr char kYoutubeDownScript[] = "youtubedown.js";

std::string GetYoutubeDownScript(const base::FilePath& path) {
  std::string contents;
  base::ReadFileToString(path, &contents);
  return contents;
}

}  // namespace

PlaylistYoutubeDownComponentManager::PlaylistYoutubeDownComponentManager(
    component_updater::ComponentUpdateService* cus)
    : component_update_service_(cus) {}

PlaylistYoutubeDownComponentManager::~PlaylistYoutubeDownComponentManager() =
    default;

void PlaylistYoutubeDownComponentManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void PlaylistYoutubeDownComponentManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void PlaylistYoutubeDownComponentManager::RegisterIfNeeded() {
  if (register_requested_)
    return;

  register_requested_ = true;
  RegisterPlaylistYoutubeDownComponent(
      component_update_service_,
      base::BindRepeating(
          &PlaylistYoutubeDownComponentManager::OnComponentReady,
          weak_factory_.GetWeakPtr()));
}

void PlaylistYoutubeDownComponentManager::OnComponentReady(
    const base::FilePath& install_path) {
  const auto youtubedown_path = install_path.AppendASCII(kYoutubeDownScript);
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&GetYoutubeDownScript, youtubedown_path),
      base::BindOnce(
          &PlaylistYoutubeDownComponentManager::OnGetYoutubeDownScript,
          weak_factory_.GetWeakPtr()));
}

void PlaylistYoutubeDownComponentManager::OnGetYoutubeDownScript(
    const std::string& script) {
  youtubedown_script_ = script;

  for (auto& observer : observer_list_)
    observer.OnYoutubeDownScriptReady(youtubedown_script_);
}

}  // namespace playlist
