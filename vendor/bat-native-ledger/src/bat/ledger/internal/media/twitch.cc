/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
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
std::pair<std::string, std::string> MediaTwitch::GetMediaIdFromParts(
    const std::map<std::string, std::string>& parts) {
  std::string id;
  std::string user_id;
  std::map<std::string, std::string>::const_iterator iter =
        parts.find("event");
  if (iter != parts.end() && parts.find("properties") != parts.end()) {
    unsigned int size = _twitch_events.size();
    for (size_t i = 0; i < size; i++) {
      if (iter->second == _twitch_events[i]) {
        iter = parts.find("channel");
        if (iter != parts.end()) {
          id = iter->second;
          user_id = id;
        }
        iter = parts.find("vod");
        if (iter != parts.end()) {
          std::string idAddition(iter->second);
          if (idAddition.find('v') != std::string::npos) {
            id += "_vod_" + braveledger_bat_helper::split(idAddition, 'v')[1];
          }
        }
      }
    }
  }
  return std::make_pair(id, user_id);
}

// static
std::string MediaTwitch::GetMediaURL(const std::string& media_id) {
  std::string res;

  if (media_id.empty()) {
    return std::string();
  }

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

// static
std::string MediaTwitch::GetLinkType(const std::string& url,
                                     const std::string& first_party_url,
                                     const std::string& referrer) {
  std::string type;
  bool is_valid_twitch_path =
      braveledger_bat_helper::HasSameDomainAndPath(
                                  url, "ttvnw.net", "/v1/segment/");

  if (
    (
      (first_party_url.find("https://www.twitch.tv/") == 0 ||
        first_party_url.find("https://m.twitch.tv/") == 0) ||
      (referrer.find("https://player.twitch.tv/") == 0)) &&
    is_valid_twitch_path
  ) {
    type = TWITCH_MEDIA_TYPE;
  }

  return type;
}

// static
std::string MediaTwitch::GetMediaIdFromUrl(
  const std::string& url,
  const std::string& publisher_blob) {
  std::string mediaId = braveledger_media::ExtractData(url, "twitch.tv/", "/");

  if (url.find("twitch.tv/videos/") != std::string::npos) {
    mediaId = braveledger_media::ExtractData(publisher_blob,
      "data-test-selector=\"videos-channel-header-item\" href=\"/", "/");
  }
  return mediaId;
}

// static
std::string MediaTwitch::GetMediaKeyFromUrl(
  const std::string& id,
  const std::string& url) {
  if (id == "twitch" || id.empty()) {
    return std::string();
  }

  if (url.find("twitch.tv/videos/") != std::string::npos) {
    std::string vod_id = braveledger_media::ExtractData(url,
                                                      "twitch.tv/videos/",
                                                      "/");
    return (std::string)TWITCH_MEDIA_TYPE + "_" + id + "_vod_" + vod_id;
  }
  return (std::string)TWITCH_MEDIA_TYPE + "_" + id;
}

// static
void MediaTwitch::UpdatePublisherData(
    std::string* publisher_name,
    std::string* publisher_favicon_url,
    const std::string& publisher_blob) {
  *publisher_name = GetPublisherName(publisher_blob);
  *publisher_favicon_url = GetFaviconUrl(publisher_blob, *publisher_name);
}

// static
std::string MediaTwitch::GetPublisherName(
    const std::string& publisher_blob) {
  return braveledger_media::ExtractData(publisher_blob,
    "<h5 class=\"\">", "</h5>");
}

// static
std::string MediaTwitch::GetFaviconUrl(
    const std::string& publisher_blob,
    const std::string& handle) {
  if (handle.empty()) {
    return std::string();
  }

  const std::string wrapper = braveledger_media::ExtractData(publisher_blob,
    "class=\"tw-avatar tw-avatar--size-36\"",
    "</figure>");

  return braveledger_media::ExtractData(wrapper, "src=\"", "\"");
}

// static
std::string MediaTwitch::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }

  return (std::string)TWITCH_MEDIA_TYPE + "#author:" + key;
}


void MediaTwitch::OnMediaActivityError(const ledger::VisitData& visit_data,
                                       uint64_t window_id) {
  std::string url = TWITCH_TLD;
  std::string name = TWITCH_MEDIA_TYPE;

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
        << TWITCH_MEDIA_TYPE << " (name: "
        << name << ", url: "
        << visit_data.url << ")";
  }
}

void MediaTwitch::ProcessMedia(const std::map<std::string, std::string>& parts,
                               const ledger::VisitData& visit_data) {
  std::pair<std::string, std::string> site_ids(GetMediaIdFromParts(parts));
  std::string media_id = site_ids.first;
  std::string user_id = site_ids.second;
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
                user_id,
                _1,
                _2));
}

void MediaTwitch::ProcessActivityFromUrl(uint64_t window_id,
                                         const ledger::VisitData& visit_data,
                                         const std::string& publisher_blob) {
  if (publisher_blob.empty() ||
      publisher_blob == ledger::kIgnorePublisherBlob) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  std::string media_id = GetMediaIdFromUrl(visit_data.url,
                                           publisher_blob);

  media_id = base::ToLowerASCII(media_id);
  std::string media_key = GetMediaKeyFromUrl(media_id, visit_data.url);

  if (media_key.empty() || media_id.empty()) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&MediaTwitch::OnMediaPublisherActivity,
                this,
                window_id,
                visit_data,
                media_key,
                media_id,
                publisher_blob,
                _1,
                _2));
}

void MediaTwitch::OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const ledger::TwitchEventInfo& twitch_info,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    const std::string& user_id,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Failed to get publisher info";
    return;
  }

  if (publisher_info) {
    ledger::TwitchEventInfo old_event;
    std::map<std::string, ledger::TwitchEventInfo>::const_iterator iter =
        twitch_events.find(media_key);
    if (iter != twitch_events.end()) {
      old_event = iter->second;
    }

    ledger::TwitchEventInfo new_event(twitch_info);
    new_event.status_ = GetTwitchStatus(old_event, new_event);

    const uint64_t real_duration = GetTwitchDuration(old_event, new_event);
    twitch_events[media_key] = new_event;

    SavePublisherInfo(real_duration,
                      std::string(),
                      publisher_info->url,
                      publisher_info->name,
                      visit_data,
                      window_id,
                      publisher_info->favicon_url,
                      std::string(),
                      publisher_info->id);
    return;
  }

  if (media_id.empty()) {
    return;
  }

  ledger::TwitchEventInfo old_event;
  auto iter = twitch_events.find(media_key);
  if (iter != twitch_events.end()) {
    old_event = iter->second;
  }

  ledger::TwitchEventInfo new_event(twitch_info);
  new_event.status_ = GetTwitchStatus(old_event, new_event);

  const uint64_t real_duration = GetTwitchDuration(old_event, new_event);
  twitch_events[media_key] = new_event;

  if (real_duration == 0) {
    return;
  }

  if (media_id.find("_vod_") != std::string::npos) {
    // VOD
    std::vector<std::string> media_props =
        braveledger_bat_helper::split(media_id, MEDIA_DELIMITER);
    if (media_props.empty()) {
      return;
    }

    std::string media_url = GetMediaURL(user_id);
    std::string oembed_url =
        (std::string)TWITCH_VOD_URL + media_props[media_props.size() - 1];

    auto callback = std::bind(&MediaTwitch::OnEmbedResponse,
                              this,
                              real_duration,
                              media_key,
                              media_url,
                              visit_data,
                              window_id,
                              user_id,
                              _1,
                              _2,
                              _3);

    const std::string url = (std::string)TWITCH_PROVIDER_URL + "?json&url=" +
        ledger_->URIEncode(oembed_url);

    FetchDataFromUrl(url, callback);
    return;
  }

  // Live stream
  SavePublisherInfo(real_duration,
                    media_key,
                    GetMediaURL(media_id),
                    media_id,
                    visit_data,
                    window_id,
                    std::string(),
                    media_id);
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
    const std::string& user_id,
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

  SavePublisherInfo(duration,
                    media_key,
                    media_url,
                    author_name,
                    visit_data,
                    window_id,
                    fav_icon,
                    user_id);
}

void MediaTwitch::OnMediaPublisherActivity(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& media_key,
    const std::string& media_id,
    const std::string& publisher_blob,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
  if (result != ledger::Result::LEDGER_OK &&
    result != ledger::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (!info || result == ledger::Result::NOT_FOUND) {
    // first see if we have the publisher a different way (VOD vs. live stream
    ledger_->GetPublisherInfo(
        GetPublisherKey(media_id),
        std::bind(&MediaTwitch::OnPublisherInfo,
                  this,
                  window_id,
                  visit_data,
                  media_key,
                  media_id,
                  publisher_blob,
                  _1,
                  _2));
  } else {
    if (info->verified && info->favicon_url.empty()) {
      std::string publisher_name;
      std::string publisher_favicon_url;

      UpdatePublisherData(&publisher_name,
                          &publisher_favicon_url,
                          publisher_blob);

      if (!publisher_favicon_url.empty()) {
        SavePublisherInfo(0,
                          media_key,
                          GetMediaURL(media_id),
                          publisher_name,
                          visit_data,
                          window_id,
                          publisher_favicon_url,
                          media_id);
        return;
      }
    }

    ledger_->OnPanelPublisherInfo(result, std::move(info), window_id);
  }
}

void MediaTwitch::OnPublisherInfo(
    uint64_t window_id,
    const ledger::VisitData visit_data,
    const std::string& media_key,
    const std::string& media_id,
    const std::string& publisher_blob,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK  &&
    result != ledger::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    std::string publisher_name;
    std::string publisher_favicon_url;

    UpdatePublisherData(&publisher_name,
                        &publisher_favicon_url,
                        publisher_blob);

    if (publisher_name.empty()) {
      publisher_name = media_id;
    }

    SavePublisherInfo(0,
                      media_key,
                      GetMediaURL(media_id),
                      publisher_name,
                      visit_data,
                      window_id,
                      publisher_favicon_url,
                      media_id);
  } else {
    ledger_->OnPanelPublisherInfo(result,
                                  std::move(publisher_info),
                                  window_id);
  }
}

void MediaTwitch::SavePublisherInfo(const uint64_t duration,
                                    const std::string& media_key,
                                    const std::string& publisher_url,
                                    const std::string& publisher_name,
                                    const ledger::VisitData& visit_data,
                                    const uint64_t window_id,
                                    const std::string& fav_icon,
                                    const std::string& channel_id,
                                    const std::string& publisher_key) {
  if (channel_id.empty() && publisher_key.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "author id is missing for: " << media_key;
    return;
  }

  std::string key = publisher_key;
  if (key.empty()) {
    key = GetPublisherKey(channel_id);
  }

  if (key.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher id is missing for: " << media_key;
    return;
  }

  ledger::VisitData updated_visit_data(visit_data);

  if (fav_icon.length() > 0) {
    updated_visit_data.favicon_url = fav_icon;
  }

  updated_visit_data.provider = TWITCH_MEDIA_TYPE;
  updated_visit_data.name = publisher_name;
  updated_visit_data.url = publisher_url + "/videos";

  ledger_->SaveMediaVisit(key,
                          updated_visit_data,
                          duration,
                          window_id);
  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, key);
  }
}

}  // namespace braveledger_media
