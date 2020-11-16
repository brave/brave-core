/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tabs/tabs.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/internal/ad_transfer/ad_transfer.h"

namespace ads {

Tabs::Tabs(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Tabs::~Tabs() = default;

bool Tabs::IsVisible(
    const int32_t id) const {
  if (id == 0) {
    return false;
  }

  return visible_tab_id_ == id;
}

void Tabs::OnUpdated(
    const int32_t id,
    const std::string& url,
    const bool is_visible,
    const bool is_incognito) {
  if (is_incognito) {
    BLOG(7, "Tab id " << id << " is incognito");
    return;
  }

  if (!is_visible) {
    BLOG(7, "Tab id " << id << " is occluded");
    return;
  }

  if (visible_tab_id_ == id) {
    return;
  }

  BLOG(2, "Tab id " << id << " is visible");

  ads_->get_user_activity()->RecordActivityForType(
      UserActivityType::kOpenedNewOrFocusedOnExistingTab);

  last_visible_tab_id_ = visible_tab_id_;

  visible_tab_id_ = id;

  TabInfo tab;
  tab.id = id;
  tab.url = url;

  tabs_.insert({id, tab});
}

void Tabs::OnClosed(
    const int32_t id) {
  BLOG(2, "Tab id " << id << " was closed");

  tabs_.erase(id);

  ads_->get_ad_transfer()->Cancel(id);

  ads_->get_user_activity()->RecordActivityForType(
      UserActivityType::kClosedTab);
}

void Tabs::OnMediaPlaying(
    const int32_t id) {
  if (tabs_[id].is_playing_media) {
    // Media is already playing for this tab
    return;
  }

  BLOG(2, "Tab id " << id << " started playing media");

  ads_->get_user_activity()->RecordActivityForType(
      UserActivityType::kStartedPlayingMedia);

  tabs_[id].is_playing_media = true;
}

void Tabs::OnMediaStopped(
    const int32_t id) {
  if (!tabs_[id].is_playing_media) {
    // Media is not playing for this tab
    return;
  }

  BLOG(2, "Tab id " << id << " stopped playing media");

  tabs_[id].is_playing_media = false;
}

bool Tabs::IsPlayingMedia(
    const int32_t id) const {
  TabInfo tab;

  if (tabs_.find(id) != tabs_.end()) {
    tab = tabs_.at(id);
  }

  return tab.is_playing_media;
}

TabInfo Tabs::GetVisible() const {
  TabInfo tab;

  if (tabs_.find(visible_tab_id_) != tabs_.end()) {
    tab = tabs_.at(visible_tab_id_);
  }

  return tab;
}

TabInfo Tabs::GetLastVisible() const {
  TabInfo tab;

  if (tabs_.find(last_visible_tab_id_) != tabs_.end()) {
    tab = tabs_.at(last_visible_tab_id_);
  }

  return tab;
}

}  // namespace ads
