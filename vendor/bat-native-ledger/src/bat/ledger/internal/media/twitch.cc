/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <vector>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/twitch.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

static const std::vector<std::string> _twitch_events = {
    "buffer-empty",
    "buffer-refill",
    "video_end",
    "minute-watched",
    "video_pause",
    "player_click_vod_seek",
    "video-play",
    "video_error"};

MediaTwitch::MediaTwitch(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

MediaTwitch::~MediaTwitch() {
}

// static
std::string MediaTwitch::GetMediaIdFromParts(
    const std::map<std::string, std::string>& parts) {
  std::map<std::string, std::string>::const_iterator iter =
        parts.find("event");
  if (iter != parts.end() && parts.find("properties") != parts.end()) {
    unsigned int size = _twitch_events.size();
    for (size_t i = 0; i < size; i++) {
      if (iter->second == _twitch_events[i]) {
        iter = parts.find("channel");
        std::string id("");
        if (iter != parts.end()) {
          id = iter->second;
        }
        iter = parts.find("vod");
        if (iter != parts.end()) {
          std::string idAddition(iter->second);
          if (idAddition.find('v') != std::string::npos) {
            id += "_vod_" + braveledger_bat_helper::split(idAddition, 'v')[1];
          }
        }

        return id;
      }
    }
  }

  return std::string();
}

// static
std::string MediaTwitch::GetMediaURL(const std::string& media_id) {
  std::string res;

  DCHECK(!media_id.empty());
  return "https://www.twitch.tv/" + media_id;
}

// static
std::string MediaTwitch::GetTwitchStatus(
    const ledger::TwitchEventInfo& old_event,
    const ledger::TwitchEventInfo& new_event) {
  std::string status = "playing";

  if (
    (
      new_event.event_ == "video_pause" &&
      old_event.event_ != "video_pause") ||
      // User clicked pause (we need to exclude seeking while paused)
    (
      new_event.event_ == "video_pause" &&
      old_event.event_ == "video_pause" &&
      old_event.status_ == "playing") ||
      // User clicked pause as soon as he clicked play
    (
      new_event.event_ == "player_click_vod_seek" &&
      old_event.status_ == "paused")
      // Seeking a video while it is paused
  ) {
    status = "paused";
  }

  // User pauses a video, then seeks it and plays it again
  if (new_event.event_ == "video_pause" &&
      old_event.event_ == "player_click_vod_seek" &&
      old_event.status_ == "paused") {
    status = "playing";
  }

  return status;
}

// static
uint64_t MediaTwitch::GetTwitchDuration(
    const ledger::TwitchEventInfo& old_event,
    const ledger::TwitchEventInfo& new_event) {
  // Remove duplicated events
  if (old_event.event_ == new_event.event_ &&
      old_event.time_ == new_event.time_) {
    return 0;
  }

  // Start event
  if (new_event.event_ == "video-play") {
    return TWITCH_MINIMUM_SECONDS;
  }

  double time = 0;
  std::stringstream tempTime(new_event.time_);
  double currentTime = 0;
  tempTime >> currentTime;
  std::stringstream tempOld(old_event.time_);
  double oldTime = 0;
  tempOld >> oldTime;

  if (old_event.event_ == "video-play") {
    time = currentTime - oldTime - TWITCH_MINIMUM_SECONDS;
  } else if (new_event.event_ == "minute-watched" ||  // Minute watched
      new_event.event_ == "buffer-empty" ||  // Run out of buffer
      new_event.event_ == "video_error" ||  // Video has some problems
      new_event.event_ == "video_end" ||  // Video ended
      (new_event.event_ == "player_click_vod_seek" &&
       old_event.status_ == "paused") ||  // Vod seek
      (
        new_event.event_ == "video_pause" &&
        (
          (
            old_event.event_ != "video_pause" &&
            old_event.event_ != "player_click_vod_seek") ||
          old_event.status_ == "playing")
      )  // User paused video
    ) {
    time = currentTime - oldTime;
  }

  if (time < 0) {
    return 0;
  }

  // if autoplay is off and play is pressed
  if (old_event.status_.empty()) {
    return 0;
  }

  if (time > TWITCH_MAXIMUM_SECONDS_CHUNK) {
    time = TWITCH_MAXIMUM_SECONDS_CHUNK;
  }

  return static_cast<uint64_t>(std::round(time));
}

void MediaTwitch::ProcessMedia(const std::map<std::string, std::string>& parts,
                               const ledger::VisitData& visit_data) {
  std::string media_id = GetMediaIdFromParts(parts);
  if (media_id.empty()) {
    return;
  }

  std::string media_key = braveledger_media::GetMediaKey(media_id,
                                                         TWITCH_MEDIA_TYPE);
  BLOG(ledger_, ledger::LogLevel::LOG_DEBUG) << "Media key: " << media_key;

  ledger::TwitchEventInfo twitch_info;
  std::map<std::string, std::string>::const_iterator iter = parts.find("event");
  if (iter != parts.end()) {
    twitch_info.event_ = iter->second;
  }

  iter = parts.find("time");
  if (iter != parts.end()) {
    twitch_info.time_ = iter->second;
  }

  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&MediaTwitch::OnMediaPublisherInfo,
                this,
                media_id,
                media_key,
                twitch_info,
                visit_data,
                0,
                _1,
                _2));
}

void MediaTwitch::OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const ledger::TwitchEventInfo& twitch_info,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Failed to get publisher info";
    return;
  }

  if (!publisher_info && !publisher_info.get()) {
    if (media_id.empty()) {
      return;
    }

    ledger::TwitchEventInfo old_event;
    std::map<std::string, ledger::TwitchEventInfo>::const_iterator iter =
        twitch_events.find(media_key);
    if (iter != twitch_events.end()) {
      old_event = iter->second;
    }

    ledger::TwitchEventInfo new_event(twitch_info);
    new_event.status_ = GetTwitchStatus(old_event, new_event);

    uint64_t real_duration = GetTwitchDuration(old_event, new_event);
    twitch_events[media_key] = new_event;

    if (real_duration == 0) {
      return;
    }

    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.favicon_url = "";
    updated_visit_data.provider = TWITCH_MEDIA_TYPE;



    if (media_id.find("_vod_") != std::string::npos) {
      // VOD
      std::vector<std::string> media_props =
          braveledger_bat_helper::split(media_id, MEDIA_DELIMITER);
      if (media_props.empty()) {
        return;
      }

      std::string new_id = media_props[0];
      std::string media_url = GetMediaURL(new_id);
      std::string oembed_url =
          (std::string)TWITCH_VOD_URL + media_props[media_props.size() - 1];
      updated_visit_data.name = new_id;
      updated_visit_data.url = media_url + "/videos";

      auto callback = std::bind(
          &MediaTwitch::OnEmbedResponse,
          this,
          real_duration,
          media_key,
          media_url,
          updated_visit_data,
          window_id,
          _1,
          _2,
          _3);

      const std::string url = (std::string)TWITCH_PROVIDER_URL + "?json&url=" +
          ledger_->URIEncode(oembed_url);

      FetchDataFromUrl(url, callback);
      return;
    }

    // Live stream
    std::string id = (std::string)TWITCH_MEDIA_TYPE + "#author:" + media_id;
    updated_visit_data.name = media_id;
    updated_visit_data.url = GetMediaURL(media_id) + "/videos";

    ledger_->SaveMediaVisit(id, updated_visit_data, real_duration, window_id);
    ledger_->SetMediaPublisherInfo(media_key, id);
  } else {
    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.name = publisher_info->name;
    updated_visit_data.url = publisher_info->url;
    updated_visit_data.provider = TWITCH_MEDIA_TYPE;
    updated_visit_data.favicon_url = publisher_info->favicon_url;

    ledger::TwitchEventInfo old_event;
    std::map<std::string, ledger::TwitchEventInfo>::const_iterator iter =
        twitch_events.find(media_key);
    if (iter != twitch_events.end()) {
      old_event = iter->second;
    }

    ledger::TwitchEventInfo new_event(twitch_info);
    new_event.status_ = GetTwitchStatus(old_event, new_event);

    uint64_t real_duration = GetTwitchDuration(old_event, new_event);
    twitch_events[media_key] = new_event;

    std::string id = publisher_info->id;
    ledger_->SaveMediaVisit(id, updated_visit_data, real_duration, window_id);
  }
}

void MediaTwitch::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   std::string(),
                   std::string(),
                   ledger::URL_METHOD::GET,
                   callback);
}

void MediaTwitch::OnEmbedResponse(
    const uint64_t duration,
    const std::string& media_key,
    const std::string& media_url,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != 200) {
    // TODO(anyone): add error handler
    return;
  }

  std::string fav_icon;
  braveledger_bat_helper::getJSONValue("author_thumbnail_url",
                                       response,
                                       &fav_icon);
  std::string author_name;
  braveledger_bat_helper::getJSONValue("author_name", response, &author_name);

  std::string twitchMediaID = visit_data.name;
  std::string id = (std::string)TWITCH_MEDIA_TYPE + "#author:" + twitchMediaID;

  ledger::VisitData updated_visit_data(visit_data);
  updated_visit_data.name = author_name;

  if (fav_icon.length() > 0) {
    updated_visit_data.favicon_url = fav_icon;
  }

  ledger_->SaveMediaVisit(id, updated_visit_data, duration, window_id);
  ledger_->SetMediaPublisherInfo(media_key, id);
}

}  // namespace braveledger_media
