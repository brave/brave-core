/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/media_detector_component_manager.h"

#include <utility>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/browser/media_detector_component_installer.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace playlist {

namespace {

using ScriptName = base::FilePath::StringType;
using ScriptToSchemefulSiteMap = base::flat_map<ScriptName, net::SchemefulSite>;
using ScriptToResourceIdMap = base::flat_map<ScriptName, int>;

const base::FilePath::StringType& GetMediaSourceAPISuppressorScriptName() {
  static const base::NoDestructor kMediaSourceApiSuppressor(
      ScriptName(FILE_PATH_LITERAL("media_source_api_suppressor.js")));
  return *kMediaSourceApiSuppressor;
}

const base::FilePath::StringType& GetBaseScriptName() {
  static const base::NoDestructor kBaseScript(
      ScriptName(FILE_PATH_LITERAL("index.js")));
  return *kBaseScript;
}

const ScriptToSchemefulSiteMap& GetScriptNameToSchemefulSiteMap() {
  static const base::NoDestructor kScriptNameToSchemefulSites(
      ScriptToSchemefulSiteMap{
          {FILE_PATH_LITERAL("youtube.com.js"),
           net::SchemefulSite(GURL("https://youtube.com"))}});

  return *kScriptNameToSchemefulSites;
}

base::flat_map<ScriptName, std::string> GetLocalScriptMap() {
  const auto& rb = ui::ResourceBundle::GetSharedInstance();
  return {
      {GetMediaSourceAPISuppressorScriptName(),
       std::string(rb.LoadDataResourceString(
           IDR_PLAYLIST_MEDIA_SOURCE_API_SUPPRESSOR_JS))},
      {GetBaseScriptName(),
       std::string(rb.LoadDataResourceString(IDR_PLAYLIST_MEDIA_DETECTOR_JS))},
      {FILE_PATH_LITERAL("youtube.com.js"),
       std::string(
           rb.LoadDataResourceString(IDR_PLAYLIST_MEDIA_DETECTOR_YOUTUBE_JS))},
  };
}

std::string ReadScript(const base::FilePath& path) {
  std::string contents;
  base::ReadFileToString(path, &contents);
  return contents;
}

base::flat_map<ScriptName, std::string> ReadScriptsFromComponent(
    const base::flat_set<base::FilePath>& files) {
  base::flat_map<ScriptName, std::string> script_map;
  for (const auto& path : files) {
    if (auto script = ReadScript(path); !script.empty()) {
      script_map[path.BaseName().value()] = script;
    }
  }

  return script_map;
}

}  // namespace

MediaDetectorComponentManager::MediaDetectorComponentManager(
    component_updater::ComponentUpdateService* component_update_service)
    : component_update_service_(component_update_service) {
  // TODO(sko) We have breaking changes and not using scripts from component
  // updater. But we should use script from the component at some point.
  SetUseLocalScript();
}

MediaDetectorComponentManager::~MediaDetectorComponentManager() = default;

void MediaDetectorComponentManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void MediaDetectorComponentManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void MediaDetectorComponentManager::MaybeInitScripts() {
  if (base_script_.empty()) {
    // In case we have yet to fetch the script, use local script instead. At the
    // same time, fetch the script from component.
    RegisterIfNeeded();
    OnGetScripts(GetLocalScriptMap());
  }
}

void MediaDetectorComponentManager::RegisterIfNeeded() {
  if (register_requested_) {
    return;
  }

  register_requested_ = true;
  RegisterMediaDetectorComponent(
      component_update_service_,
      base::BindRepeating(&MediaDetectorComponentManager::OnComponentReady,
                          weak_factory_.GetWeakPtr()));
}

void MediaDetectorComponentManager::OnComponentReady(
    const base::FilePath& install_path) {
  base::flat_set<base::FilePath> files(
      {install_path.Append(GetMediaSourceAPISuppressorScriptName()),
       install_path.Append(GetBaseScriptName())});
  for (const auto& [file, _] : GetScriptNameToSchemefulSiteMap()) {
    files.insert(install_path.Append(file));
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, base::MayBlock(),
      base::BindOnce(&ReadScriptsFromComponent, std::move(files)),
      base::BindOnce(&MediaDetectorComponentManager::OnGetScripts,
                     weak_factory_.GetWeakPtr()));
}

void MediaDetectorComponentManager::OnGetScripts(
    const MediaDetectorComponentManager::ScriptMap& script_map) {
  if (script_map.empty()) {
    LOG(ERROR) << __FUNCTION__ << " scripts are empty!";
    return;
  }

  CHECK(script_map.contains(GetMediaSourceAPISuppressorScriptName()));
  media_source_api_suppressor_ =
      script_map.at(GetMediaSourceAPISuppressorScriptName());

  CHECK(script_map.contains(GetBaseScriptName()));
  base_script_ = script_map.at(GetBaseScriptName());

  // This could have been filled when we've used media detector script before
  // component updater finishes its work.
  site_specific_detectors_.clear();

  const auto& schemeful_site_map = GetScriptNameToSchemefulSiteMap();
  for (const auto& [script_name, script] : script_map) {
    if (schemeful_site_map.contains(script_name)) {
      site_specific_detectors_[schemeful_site_map.at(script_name)] = script;
    }
  }

  for (auto& observer : observer_list_) {
    observer.OnScriptReady(base_script_);
  }
}

void MediaDetectorComponentManager::SetUseLocalScript() {
  register_requested_ = true;

  OnGetScripts(GetLocalScriptMap());
}

const std::string&
MediaDetectorComponentManager::GetMediaSourceAPISuppressorScript() {
  MaybeInitScripts();
  CHECK(!media_source_api_suppressor_.empty());
  return media_source_api_suppressor_;
}

std::string MediaDetectorComponentManager::GetMediaDetectorScript(
    const GURL& url) {
  MaybeInitScripts();

  net::SchemefulSite site(url);
  std::string detector_script = base_script_;
  DCHECK(!detector_script.empty());

  if (site_specific_detectors_.count(site)) {
    constexpr std::string_view kPlaceholder =
        "const siteSpecificDetector = null";
    auto pos = detector_script.find(kPlaceholder);
    if (pos != std::string::npos) {
      detector_script.replace(pos, kPlaceholder.length(),
                              site_specific_detectors_.at(site));
    } else {
      // Reportedly, in some environments(e.g. Android release), the js resource
      // could be minified by removing white spaces.
      constexpr std::string_view kPlaceholderWithoutWhitespace =
          "const siteSpecificDetector=null";
      pos = detector_script.find(kPlaceholderWithoutWhitespace);
      if (pos != std::string::npos) {
        detector_script.replace(pos, kPlaceholderWithoutWhitespace.length(),
                                site_specific_detectors_.at(site));
      } else {
        LOG(ERROR) << "Couldn't find `const siteSpecificDetector = null` from "
                      "base script";
      }
    }
  }

  return detector_script;
}

}  // namespace playlist
