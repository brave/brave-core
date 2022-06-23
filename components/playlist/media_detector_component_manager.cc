/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/media_detector_component_manager.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/media_detector_component_installer.h"

namespace playlist {

namespace {

base::FilePath GetScriptPath(const base::FilePath& install_path) {
  // TODO(sko) This will be replaced with new script.
  constexpr char kYoutubeDownScript[] = "youtubedown.js";
  return install_path.AppendASCII(kYoutubeDownScript);
}

std::string ReadScript(const base::FilePath& path) {
  std::string contents;
  base::ReadFileToString(path, &contents);
  return contents;
}

}  // namespace

MediaDetectorComponentManager::MediaDetectorComponentManager(
    component_updater::ComponentUpdateService* component_update_service)
    : component_update_service_(component_update_service) {}

MediaDetectorComponentManager::~MediaDetectorComponentManager() = default;

void MediaDetectorComponentManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void MediaDetectorComponentManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void MediaDetectorComponentManager::RegisterIfNeeded() {
  if (register_requested_)
    return;

  register_requested_ = true;
  RegisterMediaDetectorComponent(
      component_update_service_,
      base::BindRepeating(&MediaDetectorComponentManager::OnComponentReady,
                          weak_factory_.GetWeakPtr()));
}

void MediaDetectorComponentManager::OnComponentReady(
    const base::FilePath& install_path) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(),
      base::BindOnce(&ReadScript, GetScriptPath(install_path)),
      base::BindOnce(&MediaDetectorComponentManager::OnGetScript,
                     weak_factory_.GetWeakPtr()));
}

void MediaDetectorComponentManager::OnGetScript(const std::string& script) {
  script_ = script;

  for (auto& observer : observer_list_)
    observer.OnScriptReady(script_);
}

}  // namespace playlist
