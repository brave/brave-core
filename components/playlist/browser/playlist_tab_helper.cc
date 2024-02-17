/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_tab_helper.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/playlist/browser/playlist_background_webcontents_helper.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/features.h"
#include "components/grit/brave_components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "ui/base/l10n/l10n_util.h"

namespace playlist {

// static
void PlaylistTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents,
    playlist::PlaylistService* service) {
  if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    return;
  }

  if (!service) {
    // |service| could be null when the service is not supported for the
    // browser context.
    return;
  }

  content::WebContentsUserData<PlaylistTabHelper>::CreateForWebContents(
      contents, service);
}

// static
void PlaylistTabHelper::BindMediaResponderReceiver(
    mojo::PendingAssociatedReceiver<mojom::PlaylistMediaResponder> receiver,
    content::RenderFrameHost* rfh) {
  // TODO(sszaloki): do we have to do a service check here?
  // auto* playlist_service =
  //     playlist::PlaylistServiceFactory::GetForBrowserContext(
  //         rfh->GetBrowserContext());
  // if (!playlist_service) {
  //   // We don't support playlist on OTR profile.
  //   return;
  // }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    return;
  }

  // If `web_contents` is a background WebContents, we want to route the Mojom
  // calls to the PlaylistTabHelper it belongs to.
  auto* background_web_contents_helper =
      PlaylistBackgroundWebContentsHelper::FromWebContents(web_contents);
  auto* tab_helper = background_web_contents_helper
                         ? background_web_contents_helper->GetTabHelper()
                         : PlaylistTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return;
  }

  tab_helper->media_responder_receivers_.Bind(rfh, std::move(receiver));
}

PlaylistTabHelper::PlaylistTabHelper(content::WebContents* contents,
                                     PlaylistService* service)
    : WebContentsUserData(*contents),
      service_(service),
      media_responder_receivers_(contents, this) {
  Observe(contents);
  CHECK(service_);
  service_->AddObserver(playlist_observer_receiver_.BindNewPipeAndPassRemote());

  playlist_enabled_pref_.Init(
      kPlaylistEnabledPref,
      user_prefs::UserPrefs::Get(contents->GetBrowserContext()),
      base::BindRepeating(&PlaylistTabHelper::OnPlaylistEnabledPrefChanged,
                          weak_ptr_factory_.GetWeakPtr()));
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
  DCHECK(items.size());
  is_adding_items_ = true;

  CHECK(service_);
  service_->AddMediaFiles(
      std::move(items), kDefaultPlaylistID,
      /* can_cache= */ true,
      base::BindOnce(&PlaylistTabHelper::OnAddedItems, base::Unretained(this)));
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

  auto new_playlist = playlist::mojom::Playlist::New();
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

bool PlaylistTabHelper::ShouldExtractMediaFromBackgroundWebContents() const {
  return service_->ShouldExtractMediaFromBackgroundWebContents(found_items());
}

void PlaylistTabHelper::MaybeExtractMediaFromBackgroundWebContents(
    base::OnceCallback<void(bool)> callback) {
  if (!ShouldExtractMediaFromBackgroundWebContents()) {
    std::move(callback).Run(true);
    return;
  }

  if (background_web_contents_) {
    // in progress...
    return;
  }

  const GURL url = web_contents()->GetLastCommittedURL();

  content::WebContents::CreateParams create_params(
      web_contents()->GetBrowserContext(), nullptr);
  create_params.is_never_visible = true;
  background_web_contents_ = content::WebContents::Create(create_params);
  background_web_contents_->SetAudioMuted(true);
  PlaylistBackgroundWebContentsHelper::CreateForWebContents(
      background_web_contents_.get(), GetWeakPtr(),
      service_->GetMediaSourceAPISuppressorScript(),
      service_->GetMediaDetectorScript(url), std::move(callback));
  auto load_url_params = content::NavigationController::LoadURLParams(url);

  if (service_->ShouldUseFakeUA(url) ||
      base::FeatureList::IsEnabled(features::kPlaylistFakeUA)) {
    DVLOG(2) << __func__ << " Faked UA to detect media files";

    blink::UserAgentOverride user_agent(
        "Mozilla/5.0 (iPhone; CPU iPhone OS 13_2_3 like Mac OS X) "
        "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.3 "
        "Mobile/15E148 "
        "Safari/604.1",
        /* user_agent_metadata */ {});
    background_web_contents_->SetUserAgentOverride(
        user_agent,
        /* override_in_new_tabs= */ true);
    load_url_params.override_user_agent =
        content::NavigationController::UA_OVERRIDE_TRUE;
  }

  content::NavigationController& controller =
      background_web_contents_->GetController();
  controller.LoadURLWithParams(load_url_params);

  if (base::FeatureList::IsEnabled(features::kPlaylistFakeUA)) {
    for (int i = 0; i < controller.GetEntryCount(); ++i) {
      controller.GetEntryAtIndex(i)->SetIsOverridingUserAgent(true);
    }
  }

  media_extraction_from_background_web_contents_timer_.Start(
      FROM_HERE, base::Seconds(10),
      base::BindOnce(
          &PlaylistTabHelper::OnMediaExtractionFromBackgroundWebContentsTimeout,
          weak_ptr_factory_.GetWeakPtr()));
}

void PlaylistTabHelper::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  DVLOG(2) << __FUNCTION__;

  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  const GURL url = navigation_handle->GetURL();
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  mojo::AssociatedRemote<mojom::PlaylistRenderFrameObserverConfigurator>
      frame_observer_config;
  navigation_handle->GetRenderFrameHost()
      ->GetRemoteAssociatedInterfaces()
      ->GetInterface(&frame_observer_config);
  frame_observer_config->AddMediaDetector(
      service_->GetMediaDetectorScript(url));
}

void PlaylistTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DVLOG(2) << __FUNCTION__;

  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  if (auto old_url =
          std::exchange(target_url_, web_contents()->GetLastCommittedURL());
      old_url == target_url_) {
    return;
  }

  // We're resetting data on finish, not on start, because navigation could fail
  // or aborted.
  ResetData();

  UpdateSavedItemFromCurrentContents();
}

void PlaylistTabHelper::OnMediaDetected(base::Value media) {
  const auto render_frame_host_id =
      media_responder_receivers_.GetCurrentTargetFrame()->GetGlobalId();
  DVLOG(2) << __FUNCTION__ << " " << render_frame_host_id;

  auto* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);
  if (!render_frame_host) {
    return;
  }

  // Note that we have to go the GlobalRenderFrameHostId ==> RenderFrameHost ==>
  // WebContents route, as it works for background WebContents, too (whereas
  // just calling web_contents() does not).
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }

  DVLOG(2) << "Media:\n" << media;

  service_->OnMediaDetected(std::move(media), web_contents);

  if (web_contents == background_web_contents_.get()) {
    DVLOG(2) << "Media: from background";
    auto* background_webcontents_helper =
        PlaylistBackgroundWebContentsHelper::FromWebContents(
            background_web_contents_.get());
    CHECK(background_webcontents_helper);
    // TODO(sszaloki): bloah...
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(
            std::move(*background_webcontents_helper).GetSuccessCallback(),
            true));
    background_web_contents_.reset();
  }
}

void PlaylistTabHelper::OnItemCreated(mojom::PlaylistItemPtr item) {
  DVLOG(2) << __FUNCTION__ << " " << item->page_source.spec();
  if (item->page_source != web_contents()->GetLastCommittedURL()) {
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
    std::vector<mojom::PlaylistItemPtr> items) {
  if (items.empty()) {
    return;
  }

  OnFoundMediaFromContents(url, std::move(items));
}

void PlaylistTabHelper::ResetData() {
  background_web_contents_.reset();
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
            web_contents()->GetLastCommittedURL().GetWithoutRef();
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

void PlaylistTabHelper::OnFoundMediaFromContents(
    const GURL& url,
    std::vector<mojom::PlaylistItemPtr> items) {
  if (!*playlist_enabled_pref_) {
    return;
  }

  CHECK(web_contents());
  if (url != web_contents()->GetLastCommittedURL()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " items.size(): " << items.size();

  // There's no good heuristics on how to match an
  // `is_blob_from_media_source == false` item (in `items`) against an
  // `is_blob_from_media_source == true` item (in `found_items_`).
  // Curently, we empty `found_items_` if it contains
  // an `is_blob_from_media_source == true` item.
  if (base::ranges::find(found_items_, true,
                         &mojom::PlaylistItem::is_blob_from_media_source) !=
      found_items_.cend()) {
    decltype(found_items_)().swap(found_items_);
  }

  DVLOG(2) << __FUNCTION__ << " item count : " << items.size();

  if (IsExtractingMediaFromBackgroundWebContents()) {
    found_items_.clear();
  }

  for (auto& new_item : items) {
    const auto it = base::ranges::find_if(
        found_items_,
        [&](const auto& media_source) {
          return media_source == new_item->media_source;
        },
        &mojom::PlaylistItem::media_source);
    if (it != found_items_.cend()) {
      DVLOG(2) << "The media source with url (" << (*it)->media_source
               << ") already exists so update the data";
      *it = std::move(new_item);
    } else {
      found_items_.push_back(std::move(new_item));
    }
  }

  DVLOG(2) << __FUNCTION__ << " found_items_.size(): " << found_items_.size();
  for (auto& observer : observers_) {
    observer.OnFoundItemsChanged(found_items_);
  }
}

void PlaylistTabHelper::OnMediaExtractionFromBackgroundWebContentsTimeout() {
  if (!background_web_contents_) {
    return;
  }

  auto* background_webcontents_helper =
      PlaylistBackgroundWebContentsHelper::FromWebContents(
          background_web_contents_.get());
  CHECK(background_webcontents_helper);
  std::move(*background_webcontents_helper).GetSuccessCallback().Run(false);
  background_web_contents_.reset();

  found_items_.clear();

  for (auto& observer : observers_) {
    observer.OnFoundItemsChanged(found_items_);
  }
}

std::vector<mojom::PlaylistItemPtr> PlaylistTabHelper::GetUnsavedItems() const {
  CHECK(*playlist_enabled_pref_) << "Playlist pref must be enabled";
  if (found_items_.empty()) {
    return {};
  }

  base::flat_set<std::string> saved_items;
  base::ranges::transform(saved_items_,
                          std::inserter(saved_items, saved_items.begin()),
                          [](const auto& item) { return item->id; });

  std::vector<mojom::PlaylistItemPtr> unsaved_items;
  for (const auto& found_item : found_items_) {
    if (!base::Contains(saved_items, found_item->id)) {
      unsaved_items.push_back(found_item->Clone());
    }
  }
  return unsaved_items;
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistTabHelper);

}  // namespace playlist
