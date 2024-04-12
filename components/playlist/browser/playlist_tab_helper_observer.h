/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TAB_HELPER_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TAB_HELPER_OBSERVER_H_

#include <vector>

#include "base/observer_list_types.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"

namespace playlist {

class PlaylistTabHelperObserver : public base::CheckedObserver {
 public:
  virtual void PlaylistTabHelperWillBeDestroyed() = 0;
  virtual void OnSavedItemsChanged(
      const std::vector<mojom::PlaylistItemPtr>& items) {}
  virtual void OnFoundItemsChanged(
      const std::vector<mojom::PlaylistItemPtr>& items) {}
  virtual void OnAddedItemFromTabHelper(
      const std::vector<mojom::PlaylistItemPtr>& items) {}
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TAB_HELPER_OBSERVER_H_
