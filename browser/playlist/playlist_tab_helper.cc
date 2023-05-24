/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_tab_helper.h"

#include <utility>

#include "brave/browser/playlist/playlist_service_factory.h"
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

PlaylistTabHelper::~PlaylistTabHelper() = default;

base::CallbackListSubscription
PlaylistTabHelper::RegisterSavedItemsChangedCallback(
    ItemsChangedCallbackList::CallbackType cb) {
  return saved_items_changed_callbacks_.Add(std::move(cb));
}

base::CallbackListSubscription
PlaylistTabHelper::RegisterFoundItemsChangedCallback(
    ItemsChangedCallbackList::CallbackType cb) {
  return found_items_changed_callbacks_.Add(std::move(cb));
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

  saved_items_.push_back(std::move(item));
  saved_items_changed_callbacks_.Notify(saved_items_);
}

void PlaylistTabHelper::OnItemDeleted(const std::string& id) {
  auto iter = base::ranges::find_if(
      saved_items_, [&id](const auto& item) { return id == item->id; });
  if (iter == saved_items_.end()) {
    return;
  }

  saved_items_.erase(iter);
  saved_items_changed_callbacks_.Notify(saved_items_);
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

  saved_items_changed_callbacks_.Notify(saved_items_);
  found_items_changed_callbacks_.Notify(found_items_);
}

void PlaylistTabHelper::UpdateSavedItemFromCurrentContents() {
  CHECK(service_);
  // TODO(sko) I'm a little bit worried about potential performance issue here.
  // Should we keep a map(url, [item_id, ... , item_id]]) in PlaylistService for
  // perf improvement? We'll see this really matters.

  // This is a synchronous call, so it's safe to bind this without weak
  // reference.
  auto on_get_all_items = base::BindOnce(
      [](PlaylistTabHelper* helper, std::vector<mojom::PlaylistItemPtr> items) {
        const auto& current_url = helper->web_contents()->GetVisibleURL();
        for (const auto& item : items) {
          if (item->page_source != current_url) {
            continue;
          }

          helper->saved_items_.push_back(item->Clone());
        }

        helper->saved_items_changed_callbacks_.Notify(helper->saved_items_);
      },
      this);
  service_->GetAllPlaylistItems(std::move(on_get_all_items));
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

  found_items_changed_callbacks_.Notify(found_items_);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistTabHelper);

}  // namespace playlist
