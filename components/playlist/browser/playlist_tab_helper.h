/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TAB_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TAB_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/media_session/public/mojom/audio_focus.mojom.h"
#include "services/media_session/public/mojom/media_session.mojom.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace playlist {

class PlaylistService;
class PlaylistTabHelperObserver;

class PlaylistTabHelper
    : public content::WebContentsUserData<PlaylistTabHelper>,
      public content::WebContentsObserver,
      public media_session::mojom::MediaSessionObserver,
      public mojom::PlaylistServiceObserver {
 public:
  ~PlaylistTabHelper() override;

  const std::vector<mojom::PlaylistItemPtr>& saved_items() const {
    return saved_items_;
  }

  const std::vector<mojom::PlaylistItemPtr>& found_items() const {
    return found_items_;
  }

  std::vector<mojom::PlaylistItemPtr> GetUnsavedItems() const;

  std::vector<mojom::PlaylistPtr> GetAllPlaylists() const;

  void AddObserver(PlaylistTabHelperObserver* observer);
  void RemoveObserver(PlaylistTabHelperObserver* observer);

  bool is_adding_items() const { return is_adding_items_; }

  void AddItems(std::vector<mojom::PlaylistItemPtr> items);
  void RemoveItems(std::vector<mojom::PlaylistItemPtr> items);

  void MoveItems(std::vector<mojom::PlaylistItemPtr> items,
                 mojom::PlaylistPtr target_playlist);
  void MoveItemsToNewPlaylist(std::vector<mojom::PlaylistItemPtr> items,
                              const std::string& new_playlist_name);

  base::WeakPtr<PlaylistTabHelper> GetWeakPtr();

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void OnFocusGained();
  void OnFocusLost();

  // media_session::mojom::MediaSessionObserver:
  void MediaSessionInfoChanged(
      media_session::mojom::MediaSessionInfoPtr session_info) override {}
  void MediaSessionMetadataChanged(
      const std::optional<media_session::MediaMetadata>& metadata) override;
  void MediaSessionActionsChanged(
      const std::vector<media_session::mojom::MediaSessionAction>& action)
      override {}
  void MediaSessionImagesChanged(
      const base::flat_map<media_session::mojom::MediaSessionImageType,
                           std::vector<media_session::MediaImage>>& images)
      override;
  void MediaSessionPositionChanged(
      const std::optional<media_session::MediaPosition>& position) override {}

  // mojom::PlaylistServiceObserver:
  void OnEvent(mojom::PlaylistEvent event,
               const std::string& playlist_id) override {}
  void OnItemCreated(mojom::PlaylistItemPtr item) override;
  void OnItemAddedToList(const std::string& playlist_id,
                         const std::string& item_id) override;
  void OnItemRemovedFromList(const std::string& playlist_id,
                             const std::string& item_id) override;
  void OnItemLocalDataDeleted(const std::string& id) override;
  void OnPlaylistUpdated(mojom::PlaylistPtr playlist) override {}
  void OnItemCached(mojom::PlaylistItemPtr item) override {}
  void OnItemUpdated(mojom::PlaylistItemPtr item) override {}
  void OnMediaFileDownloadScheduled(const std::string& id) override {}
  void OnMediaFileDownloadProgressed(
      const std::string& id,
      int64_t total_bytes,
      int64_t received_bytes,
      int8_t percent_complete,
      const std::string& time_remaining) override {}
  void OnMediaFilesUpdated(const GURL& url,
                           std::vector<mojom::PlaylistItemPtr> items) override {
  }

 private:
  friend class content::WebContentsUserData<PlaylistTabHelper>;

  PlaylistTabHelper(content::WebContents* contents, PlaylistService* service);

  void ResetData();
  void UpdateSavedItemFromCurrentContents();
  void OnAddedItems(std::vector<mojom::PlaylistItemPtr> items);

  void OnPlaylistEnabledPrefChanged();

  raw_ptr<PlaylistService> service_;

  bool is_adding_items_ = false;

  std::vector<mojom::PlaylistItemPtr> saved_items_;
  std::vector<mojom::PlaylistItemPtr> found_items_;
  mojom::PlaylistItemPtr found_item_;

  base::ObserverList<PlaylistTabHelperObserver> observers_;

  mojo::Receiver<mojom::PlaylistServiceObserver> playlist_observer_receiver_{
      this};

  BooleanPrefMember playlist_enabled_pref_;

  mojo::Receiver<media_session::mojom::MediaSessionObserver>
      media_session_observer_receiver_{this};

  base::WeakPtrFactory<PlaylistTabHelper> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TAB_HELPER_H_
