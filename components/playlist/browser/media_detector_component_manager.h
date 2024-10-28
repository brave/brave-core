/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_MEDIA_DETECTOR_COMPONENT_MANAGER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_MEDIA_DETECTOR_COMPONENT_MANAGER_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "net/base/schemeful_site.h"

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace playlist {

// Installs a component extention for detecting video/audio and loads script
// for clients to inject into web contents.
class MediaDetectorComponentManager {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnScriptReady(const std::string& script) = 0;
  };

  explicit MediaDetectorComponentManager(
      component_updater::ComponentUpdateService* component_update_service);
  virtual ~MediaDetectorComponentManager();

  MediaDetectorComponentManager(const MediaDetectorComponentManager&) = delete;
  MediaDetectorComponentManager& operator=(
      const MediaDetectorComponentManager&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  const std::string& GetMediaSourceAPISuppressorScript();

  // Returns a script to get media from page. If the script isn't fetched
  // from component yet, will return a local script.
  std::string GetMediaDetectorScript(const GURL& url);

  void SetUseLocalScript();

 private:
  FRIEND_TEST_ALL_PREFIXES(MediaDetectorComponentManagerTest,
                           SitesThatNeedsURLRuleForMediaPage);

  using ScriptMap = base::flat_map</* script_name */ base::FilePath::StringType,
                                   /* contents */ std::string>;

  void MaybeInitScripts();
  void RegisterIfNeeded();
  void OnComponentReady(const base::FilePath& install_path);
  void OnGetScripts(const ScriptMap& script_map);

  bool register_requested_ = false;
  raw_ptr<component_updater::ComponentUpdateService, DanglingUntriaged>
      component_update_service_;

  std::string media_source_api_suppressor_;
  std::string base_script_;

  base::flat_map<net::SchemefulSite, std::string> site_specific_detectors_;

  base::ObserverList<Observer> observer_list_;
  base::WeakPtrFactory<MediaDetectorComponentManager> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_MEDIA_DETECTOR_COMPONENT_MANAGER_H_
