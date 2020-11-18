/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_YOUTUBEDOWN_COMPONENT_MANAGER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_YOUTUBEDOWN_COMPONENT_MANAGER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace playlist {

class PlaylistYoutubeDownComponentManager {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Called when |youtubedown_script_| is initialized or updated.
    virtual void OnYoutubeDownScriptReady(
        const std::string& youtubedown_script) = 0;
  };

  explicit PlaylistYoutubeDownComponentManager(
      component_updater::ComponentUpdateService* cus);
  virtual ~PlaylistYoutubeDownComponentManager();

  PlaylistYoutubeDownComponentManager(
      const PlaylistYoutubeDownComponentManager&) = delete;
  PlaylistYoutubeDownComponentManager& operator=(
      const PlaylistYoutubeDownComponentManager&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void RegisterIfNeeded();

  std::string youtubedown_script() const { return youtubedown_script_; }

 private:
  void OnComponentReady(const base::FilePath& install_path);
  void OnGetYoutubeDownScript(const std::string& script);

  bool register_requested_ = false;
  component_updater::ComponentUpdateService* component_update_service_;
  std::string youtubedown_script_;
  base::ObserverList<Observer> observer_list_;
  base::WeakPtrFactory<PlaylistYoutubeDownComponentManager> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_YOUTUBEDOWN_COMPONENT_MANAGER_H_
