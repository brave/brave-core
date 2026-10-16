/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_TAB_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_TAB_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace playlist {

class PlaylistService;
class PlaylistTabHelperObserver;

// Finds and saves the media of the page shown in a tab. Owned by the tab's
// features, and recreated when the tab's contents is replaced.
class PlaylistTabHelper : public content::WebContentsObserver,
                          public mojom::PlaylistServiceObserver {
 public:
  DECLARE_USER_DATA(PlaylistTabHelper);

  static PlaylistTabHelper* From(tabs::TabInterface* tab);

  // Also starts detecting the media of `web_contents`.
  static std::unique_ptr<PlaylistTabHelper> Create(
      tabs::TabInterface& tab,
      content::WebContents* web_contents,
      PlaylistService* service);

  PlaylistTabHelper(tabs::TabInterface& tab,
                    content::WebContents* web_contents,
                    PlaylistService* service);

  ~PlaylistTabHelper() override;

  // The contents this was created for, observed or not.
  content::WebContents& GetWebContents() const { return *contents_; }

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
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

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
                           std::vector<mojom::PlaylistItemPtr> items) override;

 private:
  void ResetSavedItems();
  void ResetFoundItems();
  void UpdateSavedItemFromCurrentContents();
  void OnAddedItems(std::vector<mojom::PlaylistItemPtr> items);

  void OnPlaylistEnabledPrefChanged();

  // The contents this was created for, which stays observed while Playlist is
  // enabled.
  const raw_ref<content::WebContents> contents_;
  raw_ptr<PlaylistService> service_;

  bool is_adding_items_ = false;

  std::vector<mojom::PlaylistItemPtr> saved_items_;
  std::vector<mojom::PlaylistItemPtr> found_items_;

  base::ObserverList<PlaylistTabHelperObserver> observers_;

  mojo::Receiver<mojom::PlaylistServiceObserver> playlist_observer_receiver_{
      this};

  BooleanPrefMember playlist_enabled_;

  ui::ScopedUnownedUserData<PlaylistTabHelper> scoped_unowned_user_data_;

  base::WeakPtrFactory<PlaylistTabHelper> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_TAB_HELPER_H_
