/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/media_detector_component_manager.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/browser/media_detector_component_installer.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace playlist {

namespace {

base::FilePath GetScriptPath(const base::FilePath& install_path) {
  return install_path.AppendASCII("index.js");
}

std::string ReadScript(const base::FilePath& path) {
  std::string contents;
  base::ReadFileToString(path, &contents);
  return contents;
}

const std::string& GetLocalScript() {
  static const std::string kScript =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_PLAYLIST_MEDIA_DETECTOR_JS);
  return kScript;
}

}  // namespace

MediaDetectorComponentManager::MediaDetectorComponentManager(
    component_updater::ComponentUpdateService* component_update_service)
    : component_update_service_(component_update_service) {
  // TODO(sko) This list should be dynamically updated from the playlist.
  // Once it's done, remove this line.
  SetUseLocalListToHideMediaSrcAPI();
}

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
  if (script.empty()) {
    LOG(ERROR) << __FUNCTION__ << " script is empty!";
    return;
  }

  script_ = script;

  for (auto& observer : observer_list_)
    observer.OnScriptReady(script_);
}

void MediaDetectorComponentManager::SetUseLocalScriptForTesting() {
  register_requested_ = true;

  OnGetScript(GetLocalScript());
  site_specific_detectors_[net::SchemefulSite(GURL("https://youtube.com"))] =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_PLAYLIST_MEDIA_DETECTOR_YOUTUBE_JS);
}

bool MediaDetectorComponentManager::ShouldHideMediaSrcAPI(
    const GURL& url) const {
  net::SchemefulSite schemeful_site(url);
  return base::ranges::any_of(sites_to_hide_media_src_api_,
                              [&schemeful_site](const auto& site_to_hide) {
                                return site_to_hide == schemeful_site;
                              });
}

std::string MediaDetectorComponentManager::GetMediaDetectorScript(
    const GURL& url) {
  std::string detector_script = script_;
  if (detector_script.empty()) {
    // In case we have yet to fetch the script, use local script instead. At the
    // same time, fetch the script from component.
    RegisterIfNeeded();
    detector_script = GetLocalScript();
  }

  if (net::SchemefulSite site(url); site_specific_detectors_.count(site)) {
    constexpr std::string_view kPlaceholder =
        "const siteSpecificDetector = null";
    auto pos = detector_script.find(kPlaceholder);
    if (pos != std::string::npos) {
      detector_script.replace(pos, kPlaceholder.length(),
                              site_specific_detectors_.at(site));
    }
  }

  return detector_script;
}

void MediaDetectorComponentManager::SetUseLocalListToHideMediaSrcAPI() {
  sites_to_hide_media_src_api_ = {
      {net::SchemefulSite(GURL("https://youtube.com"))}};
}

}  // namespace playlist
