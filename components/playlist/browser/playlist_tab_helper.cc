/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_tab_helper.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/values_util.h"
#include "base/ranges/algorithm.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "brave/components/playlist/common/playlist_render_frame_observer_helper.h"
#include "components/global_media_controls/public/constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/media_session_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "services/media_session/public/cpp/media_image_manager.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "ui/base/l10n/l10n_util.h"

namespace playlist {

PlaylistTabHelper::PlaylistTabHelper(content::WebContents* contents,
                                     PlaylistService* service)
    : WebContentsUserData(*contents), service_(service) {
  CHECK(service_);

  Observe(contents);
  service_->AddObserver(playlist_observer_receiver_.BindNewPipeAndPassRemote());

  playlist_enabled_pref_.Init(
      kPlaylistEnabledPref,
      user_prefs::UserPrefs::Get(contents->GetBrowserContext()),
      base::BindRepeating(&PlaylistTabHelper::OnPlaylistEnabledPrefChanged,
                          weak_ptr_factory_.GetWeakPtr()));

  content::GetMediaSessionService().BindAudioFocusManager(
      audio_focus_manager_.BindNewPipeAndPassReceiver());
  audio_focus_manager_->AddObserver(
      audio_focus_observer_receiver_.BindNewPipeAndPassRemote());
}

PlaylistTabHelper::~PlaylistTabHelper() {
  for (auto& observer : observers_) {
    observer.PlaylistTabHelperWillBeDestroyed();
  }
}

void PlaylistTabHelper::AddObserver(PlaylistTabHelperObserver* observer) {
  observers_.AddObserver(observer);
}

void PlaylistTabHelper::RemoveObserver(PlaylistTabHelperObserver* observer) {
  observers_.RemoveObserver(observer);
}

void PlaylistTabHelper::AddItems(std::vector<mojom::PlaylistItemPtr> items) {
  CHECK(*playlist_enabled_pref_) << "Playlist pref must be enabled";
  DCHECK(!is_adding_items_);
  CHECK(items.size() == 1);
  is_adding_items_ = true;

  auto callback =
      base::BindOnce(&PlaylistTabHelper::OnAddedItems, base::Unretained(this));

  auto media_player_id =
      content::MediaSession::Get(&GetWebContents())->GetActiveMediaPlayerId();
  if (!media_player_id) {
    return std::move(callback).Run({});
  }

  auto metadata = GetWebContents().GetMediaMetadataByMediaPlayerIds();
  if (!metadata.contains(*media_player_id)) {
    return std::move(callback).Run({});
  }

  items[0]->id = base::Token::CreateRandom().ToString();

  auto [url, is_media_source, duration] =
      std::move(metadata.at(*media_player_id));
  items[0]->media_path = items[0]->media_source = std::move(url);
  items[0]->is_blob_from_media_source = is_media_source;
  items[0]->duration =
      base::TimeDeltaToValue(base::Seconds(duration)).GetString();

  service_->AddMediaFiles(std::move(items), kDefaultPlaylistID,
                          /* can_cache= */ true, std::move(callback));
}

void PlaylistTabHelper::RemoveItems(std::vector<mojom::PlaylistItemPtr> items) {
  CHECK(*playlist_enabled_pref_) << "Playlist pref must be enabled";
  CHECK(service_);
  DCHECK(!items.empty());

  for (const auto& item : items) {
    DCHECK(!item->parents.empty());
    for (const auto& playlist_id : item->parents) {
      service_->RemoveItemFromPlaylist(playlist_id, item->id);
    }
  }
}

void PlaylistTabHelper::MoveItems(std::vector<mojom::PlaylistItemPtr> items,
                                  mojom::PlaylistPtr target_playlist) {
  CHECK(*playlist_enabled_pref_) << "Playlist pref must be enabled";
  for (const auto& item : items) {
    CHECK_EQ(item->parents.size(), 1u)
        << "In case an item belongs to the multiple parent playlists, this "
           "method shouldn't be used.";
    service_->MoveItem(/*from=*/item->parents.at(0),
                       /*to=*/*(target_playlist->id), item->id);
  }
}

void PlaylistTabHelper::MoveItemsToNewPlaylist(
    std::vector<mojom::PlaylistItemPtr> items,
    const std::string& new_playlist_name) {
  CHECK(*playlist_enabled_pref_) << "Playlist pref must be enabled";

  auto new_playlist = mojom::Playlist::New();
  new_playlist->name = new_playlist_name;
  service_->CreatePlaylist(
      std::move(new_playlist),
      base::BindOnce(&PlaylistTabHelper::MoveItems,
                     weak_ptr_factory_.GetWeakPtr(), std::move(items)));
}

base::WeakPtr<PlaylistTabHelper> PlaylistTabHelper::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

std::u16string PlaylistTabHelper::GetSavedFolderName() {
  CHECK(*playlist_enabled_pref_) << "Playlist pref must be enabled";

  // Use saved folder's name when all saved items belong to the single same
  // parent folder. Otherwise, returns placeholder name, which is the feature
  // name.

  CHECK(saved_items_.size()) << "Caller should check if there are saved items";
  constexpr auto* kPlaceholderName = u"Playlist";
  if (const auto& parents = saved_items_.front()->parents;
      parents.empty() || parents.size() >= 2) {
    return kPlaceholderName;
  }

  const auto parent_id = saved_items_.front()->parents.front();
  if (std::any_of(saved_items_.begin() + 1, saved_items_.end(),
                  [&parent_id](const auto& item) {
                    return item->parents.empty() || item->parents.size() >= 2 ||
                           parent_id != item->parents.front();
                  })) {
    return kPlaceholderName;
  }

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (parent_id == kDefaultPlaylistID) {
    return l10n_util::GetStringUTF16(IDS_PLAYLIST_DEFAULT_PLAYLIST_NAME);
  }
#endif

  return base::UTF8ToUTF16(service_->GetPlaylist(parent_id)->name);
}

void PlaylistTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  DVLOG(2) << __FUNCTION__;

  // We're resetting data on finish, not on start, because navigation could fail
  // or aborted.
  ResetData();

  UpdateSavedItemFromCurrentContents();
}

void PlaylistTabHelper::OnItemCreated(mojom::PlaylistItemPtr item) {
  DVLOG(2) << __FUNCTION__ << " " << item->page_source.spec();
  if (item->page_source != GetWebContents().GetLastCommittedURL()) {
    return;
  }

  if (base::ranges::find_if(saved_items_, [&item](const auto& i) {
        return i->id == item->id;
      }) != saved_items_.end()) {
    // We might have already added the item from OnAddedItem().
    return;
  }

  saved_items_.push_back(std::move(item));
  for (auto& observer : observers_) {
    observer.OnSavedItemsChanged(saved_items_);
  }
}

void PlaylistTabHelper::OnItemAddedToList(const std::string& playlist_id,
                                          const std::string& item_id) {
  auto iter = base::ranges::find_if(saved_items_, [&item_id](const auto& item) {
    return item->id == item_id;
  });

  if (iter == saved_items_.end()) {
    return;
  }

  (*iter)->parents.push_back(playlist_id);

  for (auto& observer : observers_) {
    observer.OnSavedItemsChanged(saved_items_);
  }
}

void PlaylistTabHelper::OnItemRemovedFromList(const std::string& playlist_id,
                                              const std::string& item_id) {
  auto iter = base::ranges::find_if(saved_items_, [&item_id](const auto& item) {
    return item->id == item_id;
  });

  if (iter == saved_items_.end()) {
    return;
  }

  auto& parents = (*iter)->parents;
  parents.erase(base::ranges::remove(parents, playlist_id), parents.end());

  for (auto& observer : observers_) {
    observer.OnSavedItemsChanged(saved_items_);
  }
}

void PlaylistTabHelper::OnItemLocalDataDeleted(const std::string& id) {
  DVLOG(2) << __FUNCTION__ << " " << id;
  auto iter = base::ranges::find_if(
      saved_items_, [&id](const auto& item) { return id == item->id; });
  if (iter == saved_items_.end()) {
    return;
  }

  saved_items_.erase(iter);
  for (auto& observer : observers_) {
    observer.OnSavedItemsChanged(saved_items_);
  }
}

void PlaylistTabHelper::OnMediaFilesUpdated(
    const GURL& url,
    std::vector<mojom::PlaylistItemPtr> items) {}

void PlaylistTabHelper::OnFocusGained(
    media_session::mojom::AudioFocusRequestStatePtr state) {
  CHECK(state);
  if (!state->request_id) {
    return;
  }

  auto& web_contents = GetWebContents();
  if (&web_contents !=
      content::MediaSession::GetWebContentsFromRequestId(*state->request_id)) {
    return;
  }

  if (!media_session_observer_receiver_.is_bound()) {
    content::MediaSession::Get(&web_contents)
        ->AddObserver(
            media_session_observer_receiver_.BindNewPipeAndPassRemote());
  }
}

void PlaylistTabHelper::OnFocusLost(
    media_session::mojom::AudioFocusRequestStatePtr state) {
  CHECK(state);
  if (!state->request_id.has_value()) {
    return;
  }

  if (&GetWebContents() !=
      content::MediaSession::GetWebContentsFromRequestId(*state->request_id)) {
    return;
  }

  CHECK(media_session_observer_receiver_.is_bound());
  media_session_observer_receiver_.reset();

  found_item_ = mojom::PlaylistItem::New();
  found_item_->page_source = found_item_->page_redirected =
      GetWebContents().GetLastCommittedURL();
  found_items_.clear();
  for (auto& observer : observers_) {
    observer.OnFoundItemsChanged(found_items_);
  }
}

void PlaylistTabHelper::MediaSessionMetadataChanged(
    const std::optional<media_session::MediaMetadata>& metadata) {
  if (!metadata) {
    return;
  }

  if (!metadata->title.empty()) {
    found_item_->name = base::UTF16ToUTF8(metadata->title);
    found_items_.clear();
    found_items_.push_back(found_item_->Clone());
    for (auto& observer : observers_) {
      observer.OnFoundItemsChanged(found_items_);
    }
  }
}

void PlaylistTabHelper::MediaSessionImagesChanged(
    const base::flat_map<media_session::mojom::MediaSessionImageType,
                         std::vector<media_session::MediaImage>>& images) {
  media_session::MediaImageManager manager(
      global_media_controls::kMediaItemArtworkMinSize,
      global_media_controls::kMediaItemArtworkDesiredSize);

  std::optional<media_session::MediaImage> image;
  if (const auto cit =
          images.find(media_session::mojom::MediaSessionImageType::kArtwork);
      cit != images.cend()) {
    image = manager.SelectImage(cit->second);
  }

  if (image) {
    found_item_->thumbnail_source = found_item_->thumbnail_path =
        std::move(image->src);
    found_items_.clear();
    found_items_.push_back(found_item_->Clone());
    for (auto& observer : observers_) {
      observer.OnFoundItemsChanged(found_items_);
    }
  }
}

void PlaylistTabHelper::ResetData() {
  found_item_ = mojom::PlaylistItem::New();
  found_item_->page_source = found_item_->page_redirected =
      GetWebContents().GetLastCommittedURL();

  saved_items_.clear();
  found_items_.clear();

  for (auto& observer : observers_) {
    observer.OnSavedItemsChanged(saved_items_);
    observer.OnFoundItemsChanged(found_items_);
  }
}

void PlaylistTabHelper::UpdateSavedItemFromCurrentContents() {
  CHECK(service_);
  // TODO(sko) I'm a little bit worried about potential performance issue here.
  // Should we keep a map(url, [item_id, ... , item_id]]) in PlaylistService for
  // perf improvement? We'll see this really matters.

  bool should_notify = false;
  base::ranges::for_each(
      service_->GetAllPlaylistItems(),
      [this, &should_notify](const auto& item) {
        const auto& current_url =
            GetWebContents().GetLastCommittedURL().GetWithoutRef();
        const GURL page_source_url = GURL(item->page_source).GetWithoutRef();
        if (page_source_url != current_url) {
          return;
        }

        DVLOG(2) << __FUNCTION__ << " " << item->page_source.spec() << " "
                 << item->media_source.spec();
        saved_items_.push_back(item->Clone());
        should_notify = true;
      });

  if (!should_notify) {
    return;
  }

  for (auto& observer : observers_) {
    observer.OnSavedItemsChanged(saved_items_);
  }
}

std::vector<mojom::PlaylistItemPtr> PlaylistTabHelper::GetUnsavedItems() const {
  CHECK(*playlist_enabled_pref_) << "Playlist pref must be enabled";
  std::vector<mojom::PlaylistItemPtr> found_items;
  base::ranges::transform(found_items_, std::back_inserter(found_items),
                          &mojom::PlaylistItemPtr::Clone);
  return found_items;
}

std::vector<mojom::PlaylistPtr> PlaylistTabHelper::GetAllPlaylists() const {
  return service_->GetAllPlaylists();
}

void PlaylistTabHelper::OnAddedItems(
    std::vector<mojom::PlaylistItemPtr> items) {
  if (!*playlist_enabled_pref_) {
    return;
  }

  // mojo based observer tends to be notified later. i.e. OnItemCreated() will
  // be notified later than this.
  for (const auto& item : items) {
    saved_items_.push_back(item.Clone());
  }

  for (auto& observer : observers_) {
    observer.OnAddedItemFromTabHelper(items);
    observer.OnSavedItemsChanged(saved_items_);
  }

  // Reset the bit after notifying so as to prevent reentrance.
  is_adding_items_ = false;
}

void PlaylistTabHelper::OnPlaylistEnabledPrefChanged() {
  if (*playlist_enabled_pref_) {
    // It's okay to call Observe() repeatedly.
    Observe(&GetWebContents());
  } else {
    Observe(nullptr);
    ResetData();
  }
}

void PlaylistTabHelper::OnGetLoadedUrl(
    std::vector<mojom::PlaylistItemPtr> items,
    const GURL& url,
    bool is_media_source) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistTabHelper);

}  // namespace playlist
