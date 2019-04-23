/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "bat/ledger/internal/bat_get_media.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_bat_get_media {

BatGetMedia::BatGetMedia(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger),
  media_youtube_(new braveledger_media::MediaYouTube(ledger)),
  media_twitch_(new braveledger_media::MediaTwitch(ledger)),
  media_twitter_(new braveledger_media::MediaTwitter(ledger)) {
}

BatGetMedia::~BatGetMedia() {}

std::string BatGetMedia::GetLinkType(const std::string& url,
                                     const std::string& first_party_url,
                                     const std::string& referrer) {
  std::string type;
  type = braveledger_media::MediaYouTube::GetLinkType(url);

  if (type.empty()) {
    type = braveledger_media::MediaTwitch::GetLinkType(url,
                                                       first_party_url,
                                                       referrer);
  }

  return type;
}

void BatGetMedia::ProcessMedia(const std::map<std::string, std::string>& parts,
                               const std::string& type,
                               const ledger::VisitData& visit_data) {
  if (parts.size() == 0 || !ledger_->GetRewardsMainEnabled()) {
    return;
  }

  if (type == YOUTUBE_MEDIA_TYPE) {
    media_youtube_->ProcessMedia(parts, visit_data);
    return;
  }

  if (type == TWITCH_MEDIA_TYPE) {
    media_twitch_->ProcessMedia(parts, visit_data);
    return;
  }
}

void BatGetMedia::GetMediaActivityFromUrl(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& type,
    const std::string& publisher_blob) {
  if (type == YOUTUBE_MEDIA_TYPE) {
    media_youtube_->ProcessActivityFromUrl(window_id, visit_data);
  } else if (type == TWITCH_MEDIA_TYPE) {
    media_twitch_->ProcessActivityFromUrl(window_id,
                                          visit_data,
                                          publisher_blob);
  } else {
    OnMediaActivityError(visit_data, type, window_id);
  }
}

void BatGetMedia::OnMediaActivityError(const ledger::VisitData& visit_data,
                                       const std::string& type,
                                       uint64_t window_id) {
  std::string url;
  std::string name;
  if (type == YOUTUBE_MEDIA_TYPE) {
    url = YOUTUBE_TLD;
    name = YOUTUBE_MEDIA_TYPE;
  } else if (type == TWITCH_MEDIA_TYPE) {
    url = TWITCH_TLD;
    name = TWITCH_MEDIA_TYPE;
  }

  if (!url.empty()) {
    ledger::VisitData new_data;
    new_data.domain = url;
    new_data.url = "https://" + url;
    new_data.path = "/";
    new_data.name = name;

    ledger_->GetPublisherActivityFromUrl(window_id, new_data, std::string());
  } else {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Media activity error for "
        << type << " (name: "
        << name << ", url: "
        << visit_data.url << ")";
  }
}

void BatGetMedia::SaveMediaInfo(const std::string& type,
                                const std::map<std::string, std::string>& data,
                                ledger::SaveMediaInfoCallback callback) {
  if (type == TWITTER_MEDIA_TYPE) {
    media_twitter_->SaveMediaInfo(data, callback);
    return;
  }
}

}  // namespace braveledger_bat_get_media
