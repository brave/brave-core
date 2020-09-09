/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/bat_helper.h"
#include "bat/ledger/internal/legacy/media/twitch.h"
#include "bat/ledger/internal/legacy/static_values.h"
#include "net/http/http_status_code.h"

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

Twitch::Twitch(ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

Twitch::~Twitch() {
}

// static
std::pair<std::string, std::string> Twitch::GetMediaIdFromParts(
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
            auto additional_ids = base::SplitString(
                idAddition,
                "v",
                base::TRIM_WHITESPACE,
                base::SPLIT_WANT_NONEMPTY);
            if (additional_ids.size() == 1) {
              id += "_vod_" + additional_ids[0];
            }
          }
        }
      }
    }
  }
  return std::make_pair(id, user_id);
}

// static
std::string Twitch::GetMediaURL(const std::string& media_id) {
  std::string res;

  if (media_id.empty()) {
    return std::string();
  }

  return "https://www.twitch.tv/" + media_id;
}

// static
std::string Twitch::GetTwitchStatus(
    const ledger::type::MediaEventInfo& old_event,
    const ledger::type::MediaEventInfo& new_event) {
  std::string status = "playing";

  if (
    (
      new_event.event == "video_pause" &&
      old_event.event != "video_pause") ||
      // User clicked pause (we need to exclude seeking while paused)
    (
      new_event.event == "video_pause" &&
      old_event.event == "video_pause" &&
      old_event.status == "playing") ||
      // User clicked pause as soon as he clicked play
    (
      new_event.event == "player_click_vod_seek" &&
      old_event.status == "paused")
      // Seeking a video while it is paused
  ) {
    status = "paused";
  }

  // User pauses a video, then seeks it and plays it again
  if (new_event.event == "video_pause" &&
      old_event.event == "player_click_vod_seek" &&
      old_event.status == "paused") {
    status = "playing";
  }

  return status;
}

// static
uint64_t Twitch::GetTwitchDuration(
    const ledger::type::MediaEventInfo& old_event,
    const ledger::type::MediaEventInfo& new_event) {
  // Remove duplicated events
  if (old_event.event == new_event.event &&
      old_event.time == new_event.time) {
    return 0;
  }

  // Start event
  if (new_event.event == "video-play") {
    return TWITCH_MINIMUM_SECONDS;
  }

  double time = 0;
  std::stringstream tempTime(new_event.time);
  double currentTime = 0;
  tempTime >> currentTime;
  std::stringstream tempOld(old_event.time);
  double oldTime = 0;
  tempOld >> oldTime;

  if (old_event.event == "video-play") {
    time = currentTime - oldTime - TWITCH_MINIMUM_SECONDS;
  } else if (new_event.event == "minute-watched" ||  // Minute watched
      new_event.event == "buffer-empty" ||  // Run out of buffer
      new_event.event == "video_error" ||  // Video has some problems
      new_event.event == "video_end" ||  // Video ended
      (new_event.event == "player_click_vod_seek" &&
       old_event.status == "paused") ||  // Vod seek
      (
        new_event.event == "video_pause" &&
        (
          (
            old_event.event != "video_pause" &&
            old_event.event != "player_click_vod_seek") ||
          old_event.status == "playing")
      )  // User paused video
    ) {
    time = currentTime - oldTime;
  }

  if (time < 0) {
    return 0;
  }

  // if autoplay is off and play is pressed
  if (old_event.status.empty()) {
    return 0;
  }

  if (time > TWITCH_MAXIMUM_SECONDS_CHUNK) {
    time = TWITCH_MAXIMUM_SECONDS_CHUNK;
  }

  return static_cast<uint64_t>(std::round(time));
}

// static
std::string Twitch::GetLinkType(const std::string& url,
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
std::string Twitch::GetMediaIdFromUrl(
  const std::string& url,
  const std::string& publisher_blob) {
  std::string mediaId = braveledger_media::ExtractData(url, "twitch.tv/", "/");

  if (url.find("twitch.tv/videos/") != std::string::npos) {
    mediaId = braveledger_media::ExtractData(publisher_blob,
      "data-a-target=\"videos-channel-header-item\" href=\"/", "/");
  }
  return mediaId;
}

// static
std::string Twitch::GetMediaKeyFromUrl(
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
void Twitch::UpdatePublisherData(
    std::string* publisher_name,
    std::string* publisher_favicon_url,
    const std::string& publisher_blob) {
  *publisher_name = GetPublisherName(publisher_blob);
  *publisher_favicon_url = GetFaviconUrl(publisher_blob, *publisher_name);
}

// static
std::string Twitch::GetPublisherName(
    const std::string& publisher_blob) {
  return braveledger_media::ExtractData(publisher_blob,
    "<h5 class>", "</h5>");
}

// static
std::string Twitch::GetFaviconUrl(
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
std::string Twitch::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }

  return (std::string)TWITCH_MEDIA_TYPE + "#author:" + key;
}


void Twitch::OnMediaActivityError(const ledger::type::VisitData& visit_data,
                                       uint64_t window_id) {
  std::string url = TWITCH_TLD;
  std::string name = TWITCH_MEDIA_TYPE;

  if (!url.empty()) {
    ledger::type::VisitData new_visit_data;
    new_visit_data.domain = url;
    new_visit_data.url = "https://" + url;
    new_visit_data.path = "/";
    new_visit_data.name = name;

    ledger_->publisher()->GetPublisherActivityFromUrl(
        window_id, ledger::type::VisitData::New(new_visit_data), std::string());
  } else {
      BLOG(0, "Media activity error for " << TWITCH_MEDIA_TYPE << " (name: "
          << name << ", url: " << visit_data.url << ")");
  }
}

void Twitch::ProcessMedia(const std::map<std::string, std::string>& parts,
                               const ledger::type::VisitData& visit_data) {
  std::pair<std::string, std::string> site_ids(GetMediaIdFromParts(parts));
  std::string media_id = site_ids.first;
  std::string user_id = site_ids.second;
  if (media_id.empty()) {
    return;
  }

  std::string media_key = braveledger_media::GetMediaKey(media_id,
                                                         TWITCH_MEDIA_TYPE);

  ledger::type::MediaEventInfo twitch_info;
  std::map<std::string, std::string>::const_iterator iter = parts.find("event");
  if (iter != parts.end()) {
    twitch_info.event = iter->second;
  }

  iter = parts.find("time");
  if (iter != parts.end()) {
    twitch_info.time = iter->second;
  }

  ledger_->database()->GetMediaPublisherInfo(media_key,
      std::bind(&Twitch::OnMediaPublisherInfo,
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

void Twitch::ProcessActivityFromUrl(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data,
    const std::string& publisher_blob) {
  if (publisher_blob.empty() ||
      publisher_blob == ledger::constant::kIgnorePublisherBlob) {
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

  ledger_->database()->GetMediaPublisherInfo(
      media_key,
      std::bind(&Twitch::OnMediaPublisherActivity,
                this,
                window_id,
                visit_data,
                media_key,
                media_id,
                publisher_blob,
                _1,
                _2));
}

void Twitch::OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const ledger::type::MediaEventInfo& twitch_info,
    const ledger::type::VisitData& visit_data,
    const uint64_t window_id,
    const std::string& user_id,
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr publisher_info) {
  if (result != ledger::type::Result::LEDGER_OK &&
      result != ledger::type::Result::NOT_FOUND) {
    BLOG(0, "Failed to get publisher info");
    return;
  }

  if (publisher_info) {
    ledger::type::MediaEventInfo old_event;
    std::map<std::string, ledger::type::MediaEventInfo>::const_iterator iter =
        twitch_events.find(media_key);
    if (iter != twitch_events.end()) {
      old_event = iter->second;
    }

    ledger::type::MediaEventInfo new_event(twitch_info);
    new_event.status = GetTwitchStatus(old_event, new_event);

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

  ledger::type::MediaEventInfo old_event;
  auto iter = twitch_events.find(media_key);
  if (iter != twitch_events.end()) {
    old_event = iter->second;
  }

  ledger::type::MediaEventInfo new_event(twitch_info);
  new_event.status = GetTwitchStatus(old_event, new_event);

  const uint64_t real_duration = GetTwitchDuration(old_event, new_event);
  twitch_events[media_key] = new_event;

  if (real_duration == 0) {
    return;
  }

  if (media_id.find("_vod_") != std::string::npos) {
    // VOD
    auto media_props = base::SplitString(
        media_id,
        MEDIA_DELIMITER,
        base::TRIM_WHITESPACE,
        base::SPLIT_WANT_NONEMPTY);

    if (media_props.empty()) {
      return;
    }

    std::string oembed_url =
        (std::string)TWITCH_VOD_URL + media_props[media_props.size() - 1];

    auto callback = std::bind(&Twitch::OnEmbedResponse,
                              this,
                              real_duration,
                              media_key,
                              visit_data,
                              window_id,
                              user_id,
                              _1);

    const std::string url = (std::string)TWITCH_PROVIDER_URL + "?json&url=" +
        ledger_->ledger_client()->URIEncode(oembed_url);

    FetchDataFromUrl(url, callback);
    return;
  }

  // Live stream
  SavePublisherInfo(real_duration,
                    media_key,
                    "",
                    media_id,
                    visit_data,
                    window_id,
                    std::string(),
                    media_id);
}

void Twitch::FetchDataFromUrl(
    const std::string& url,
    ledger::client::LoadURLCallback callback) {
  auto request = ledger::type::UrlRequest::New();
  request->url = url;
  request->skip_log = true;
  ledger_->LoadURL(std::move(request), callback);
}

void Twitch::OnEmbedResponse(
    const uint64_t duration,
    const std::string& media_key,
    const ledger::type::VisitData& visit_data,
    const uint64_t window_id,
    const std::string& user_id,
    const ledger::type::UrlResponse& response) {
  if (response.status_code != net::HTTP_OK) {
    // TODO(anyone): add error handler
    return;
  }

  std::string fav_icon;
  braveledger_bat_helper::getJSONValue(
      "author_thumbnail_url",
      response.body,
      &fav_icon);
  std::string author_name;
  braveledger_bat_helper::getJSONValue(
      "author_name",
      response.body,
      &author_name);

  SavePublisherInfo(duration,
                    media_key,
                    "",
                    author_name,
                    visit_data,
                    window_id,
                    fav_icon,
                    user_id);
}

void Twitch::OnMediaPublisherActivity(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data,
    const std::string& media_key,
    const std::string& media_id,
    const std::string& publisher_blob,
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr info) {
  if (result != ledger::type::Result::LEDGER_OK &&
    result != ledger::type::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (!info || result == ledger::type::Result::NOT_FOUND) {
    // first see if we have the publisher a different way (VOD vs. live stream
    ledger_->database()->GetPublisherInfo(
        GetPublisherKey(media_id),
        std::bind(&Twitch::OnPublisherInfo,
                  this,
                  window_id,
                  visit_data,
                  media_key,
                  media_id,
                  publisher_blob,
                  _1,
                  _2));
  } else {
    const auto add = ledger_->publisher()->IsConnectedOrVerified(
        info->status);
    if (add && info->favicon_url.empty()) {
      std::string publisher_name;
      std::string publisher_favicon_url;

      UpdatePublisherData(&publisher_name,
                          &publisher_favicon_url,
                          publisher_blob);

      if (!publisher_favicon_url.empty()) {
        SavePublisherInfo(0,
                          media_key,
                          "",
                          publisher_name,
                          visit_data,
                          window_id,
                          publisher_favicon_url,
                          media_id);
        return;
      }
    }

    ledger_->ledger_client()->OnPanelPublisherInfo(
        result,
        std::move(info),
        window_id);
  }
}

void Twitch::OnPublisherInfo(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data,
    const std::string& media_key,
    const std::string& media_id,
    const std::string& publisher_blob,
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr publisher_info) {
  if (result != ledger::type::Result::LEDGER_OK  &&
    result != ledger::type::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (!publisher_info || result == ledger::type::Result::NOT_FOUND) {
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
                      "",
                      publisher_name,
                      visit_data,
                      window_id,
                      publisher_favicon_url,
                      media_id);
  } else {
    ledger_->ledger_client()->OnPanelPublisherInfo(
        result,
        std::move(publisher_info),
        window_id);
  }
}

void Twitch::SavePublisherInfo(const uint64_t duration,
                               const std::string& media_key,
                               const std::string& publisher_url,
                               const std::string& publisher_name,
                               const ledger::type::VisitData& visit_data,
                               const uint64_t window_id,
                               const std::string& fav_icon,
                               const std::string& channel_id,
                               const std::string& publisher_key) {
  if (channel_id.empty() && publisher_key.empty()) {
    BLOG(0, "author id is missing for: " << media_key);
    return;
  }

  std::string key = publisher_key;
  if (key.empty()) {
    key = GetPublisherKey(channel_id);
  }

  if (key.empty()) {
    BLOG(0, "Publisher id is missing for: " << media_key);
    return;
  }

  ledger::type::VisitData new_visit_data;
  if (fav_icon.length() > 0) {
    new_visit_data.favicon_url = fav_icon;
  }

  std::string url = publisher_url;
  if (url.empty()) {
    url = GetMediaURL(channel_id) + "/videos";
  }

  new_visit_data.provider = TWITCH_MEDIA_TYPE;
  new_visit_data.name = publisher_name;
  new_visit_data.url = url;

  ledger_->publisher()->SaveVideoVisit(
      key,
      new_visit_data,
      duration,
      true,
      window_id,
      [](ledger::type::Result, ledger::type::PublisherInfoPtr) {});

  if (!media_key.empty()) {
    ledger_->database()->SaveMediaPublisherInfo(
        media_key,
        key,
        [](const ledger::type::Result) {});
  }
}

}  // namespace braveledger_media
