/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_CONTROLLER_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_CONTROLLER_OBSERVER_H_

#include "base/observer_list_types.h"
#include "brave/components/playlists/browser/playlists_types.h"

class PlaylistsControllerObserver : public base::CheckedObserver {
 public:
  // |initialized| is false when init failed.
  virtual void OnPlaylistsInitialized(bool initialized) = 0;
  virtual void OnPlaylistsChanged(const PlaylistsChangeParams& params) = 0;
  virtual void OnPlaylistsDownloadRequested(const std::string& url) = 0;

 protected:
  ~PlaylistsControllerObserver() override {}
};

#endif  // BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_CONTROLLER_OBSERVER_H_
