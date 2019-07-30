/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_SERVICE_H_
#define BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_SERVICE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "brave/components/playlists/browser/playlists_controller_observer.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

class PlaylistsController;
class Profile;

class PlaylistsService : public KeyedService,
                         public PlaylistsControllerObserver {
 public:
  explicit PlaylistsService(content::BrowserContext* context);
  ~PlaylistsService() override;

  bool Init();

  PlaylistsController* controller() const { return controller_.get(); }

 private:
  void OnBaseDirectoryReady(bool ready);

  // PlaylistsControllerObserver overrides:
  void OnPlaylistsInitialized(bool initialized) override;
  void OnPlaylistsChanged(const PlaylistsChangeParams& params) override;
  void OnPlaylistsDownloadRequested(const std::string& url) override;

  base::SequencedTaskRunner* file_task_runner();

  ScopedObserver<PlaylistsController, PlaylistsControllerObserver> observer_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  base::FilePath base_dir_;
  content::BrowserContext* context_;
  std::unique_ptr<PlaylistsController> controller_;

  base::WeakPtrFactory<PlaylistsService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PlaylistsService);
};

#endif  // BRAVE_COMPONENTS_PLAYLISTS_BROWSER_PLAYLISTS_SERVICE_H_
