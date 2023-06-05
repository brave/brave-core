/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_tab_helper.h"

#include <utility>

#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/playlist/playlist_tab_helper_observer.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/features.h"
#include "content/public/browser/navigation_handle.h"

namespace playlist {

// static
void PlaylistTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    return;
  }

  // |service| could be null when the service is not supported for the
  // browser context.
  if (auto* service = PlaylistServiceFactory::GetForBrowserContext(
          contents->GetBrowserContext())) {
    content::WebContentsUserData<PlaylistTabHelper>::CreateForWebContents(
        contents, service);
  }
}

PlaylistTabHelper::PlaylistTabHelper(content::WebContents* contents,
                                     PlaylistService* service)
    : WebContentsUserData(*contents), service_(service) {
  Observe(contents);
  CHECK(service_);
  service_->AddObserver(playlist_observer_receiver_.BindNewPipeAndPassRemote());
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
  DCHECK(!is_adding_items_);
  DCHECK(items.size());
  is_adding_items_ = true;

  auto* contents = web_contents();
  DCHECK(contents);

  auto* service = PlaylistServiceFactory::GetForBrowserContext(
      contents->GetBrowserContext());
  DCHECK(service);

  service->AddMediaFiles(
      std::move(items), kDefaultPlaylistID,
      /* can_cache= */ true,
      base::BindOnce(&PlaylistTabHelper::OnAddedItems, base::Unretained(this)));
}

base::WeakPtr<PlaylistTabHelper> PlaylistTabHelper::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PlaylistTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DVLOG(2) << __FUNCTION__;

  if (auto old_url = std::exchange(target_url, web_contents()->GetVisibleURL());
      old_url == target_url) {
    return;
  }

  // We're resetting data on finish, not on start, because navigation could fail
  // or aborted.
  ResetData();

  UpdateSavedItemFromCurrentContents();

  if (navigation_handle->IsSameDocument() ||
      navigation_handle->IsServedFromBackForwardCache()) {
    FindMediaFromCurrentContents();
  }  // else DOMContentLoaded() will trigger FindMediaFromCurrentContents()
}

void PlaylistTabHelper::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  DVLOG(2) << __FUNCTION__;

  FindMediaFromCurrentContents();
}

void PlaylistTabHelper::OnItemCreated(mojom::PlaylistItemPtr item) {
  DVLOG(2) << __FUNCTION__ << " " << item->page_source.spec();
  if (item->page_source != web_contents()->GetVisibleURL()) {
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

void PlaylistTabHelper::OnItemDeleted(const std::string& id) {
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
  OnFoundMediaFromContents(url, std::move(items));
}

void PlaylistTabHelper::ResetData() {
  saved_items_.clear();
  found_items_.clear();
  sent_find_media_request_ = false;

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
  base::ranges::for_each(service_->GetAllPlaylistItems(),
                         [this, &should_notify](const auto& item) {
                           const auto& current_url =
                               web_contents()->GetVisibleURL();
                           if (item->page_source != current_url) {
                             return;
                           }

                           DVLOG(2) << __FUNCTION__ << " "
                                    << item->page_source.spec() << " "
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

void PlaylistTabHelper::FindMediaFromCurrentContents() {
  if (sent_find_media_request_) {
    return;
  }

  CHECK(service_);

  service_->FindMediaFilesFromContents(
      web_contents(),
      base::BindOnce(&PlaylistTabHelper::OnFoundMediaFromContents,
                     weak_ptr_factory_.GetWeakPtr()));

  sent_find_media_request_ = true;
}

void PlaylistTabHelper::OnFoundMediaFromContents(
    const GURL& url,
    std::vector<mojom::PlaylistItemPtr> items) {
  if (url != web_contents()->GetVisibleURL()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " item count : " << items.size();

  found_items_.insert(found_items_.end(),
                      std::make_move_iterator(items.begin()),
                      std::make_move_iterator(items.end()));

  for (auto& observer : observers_) {
    observer.OnFoundItemsChanged(found_items_);
  }
}

void PlaylistTabHelper::OnAddedItems(
    std::vector<mojom::PlaylistItemPtr> items) {
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistTabHelper);

}  // namespace playlist
