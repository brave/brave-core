/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/vimeo.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

Vimeo::Vimeo(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

Vimeo::~Vimeo() {
}

// static
std::string Vimeo::GetLinkType(const std::string& url) {
  const std::string api = "https://fresnel.vimeocdn.com/add/player-stats?";
  std::string type;

  if (!url.empty() && url.find(api) != std::string::npos) {
    type = VIMEO_MEDIA_TYPE;
  }

  return type;
}

// static
std::string Vimeo::GetVideoUrl(const std::string& video_id) {
  if (video_id.empty()) {
    return std::string();
  }

  return "https://vimeo.com/" + video_id;
}

// static
std::string Vimeo::GetMediaKey(const std::string& video_id,
                               const std::string& type) {
  if (video_id.empty()) {
    return std::string();
  }

  if (type == "vimeo-vod") {
    return (std::string)VIMEO_MEDIA_TYPE  + "_" + video_id;
  }

  return std::string();
}

// static
std::string Vimeo::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }

  return (std::string)VIMEO_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string Vimeo::GetIdFromVideoPage(const std::string& data) {
  if (data.empty()) {
    return "";
  }

  return braveledger_media::ExtractData(data,
      "\\/i.vimeocdn.com\\/portrait\\/", "_75x75");
}

// static
std::string Vimeo::GenerateFaviconUrl(const std::string& id) {
  if (id.empty()) {
    return std::string();
  }

  return base::StringPrintf("https://i.vimeocdn.com/portrait/%s_300x300.webp",
                            id.c_str());
}

// static
std::string Vimeo::GetNameFromVideoPage(const std::string& data) {
  if (data.empty()) {
    return std::string();
  }

  const std::string name =
      braveledger_media::ExtractData(data, ",\"display_name\":\"", "\"");

  std::string publisher_name;
  const std::string publisher_json = "{\"brave_publisher\":\"" +
      name + "\"}";
  // scraped data could come in with JSON code points added.
  // Make to JSON object above so we can decode.
  braveledger_bat_helper::getJSONValue(
      "brave_publisher", publisher_json, &publisher_name);
  return publisher_name;
}

// static
std::string Vimeo::GetPublisherUrl(const std::string& data) {
  if (data.empty()) {
    return std::string();
  }

  const std::string wrapper = braveledger_media::ExtractData(data,
      "<span class=\"userlink userlink--md\">", "</span>");

  const std::string name = braveledger_media::ExtractData(wrapper,
      "<a href=\"/", "\">");

  if (name.empty()) {
    return std::string();
  }

  return base::StringPrintf("https://vimeo.com/%s/videos",
                            name.c_str());
}

// static
bool Vimeo::AllowedEvent(const std::string& event) {
  if (event.empty()) {
    return false;
  }

  const std::vector<std::string> allowed = {
    "video-start-time",
    "video-minute-watched",
    "video-paused",
    "video-played",
    "video-seek",
    "video-seeked"};

  auto it = std::find(allowed.begin(), allowed.end(), event);
  return it != allowed.end();
}

// static
uint64_t Vimeo::GetDuration(
    const ledger::MediaEventInfo& old_event,
    const ledger::MediaEventInfo& new_event) {
  // Remove duplicated events
  if (old_event.event_ == new_event.event_ &&
      old_event.time_ == new_event.time_) {
    return 0u;
  }

  double time = 0.0;
  std::stringstream tempNew(new_event.time_);
  double newTime = 0.0;
  tempNew >> newTime;

  // Video started
  if (new_event.event_ == "video-start-time") {
    time = newTime;
  } else {
    std::stringstream tempOld(old_event.time_);
    double oldTime = 0;
    tempOld >> oldTime;

    if (new_event.event_ == "video-minute-watched" ||
        new_event.event_ == "video-paused") {
      time = newTime - oldTime;
    }
  }

  return static_cast<uint64_t>(std::round(time));
}

void Vimeo::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   std::string(),
                   std::string(),
                   ledger::URL_METHOD::GET,
                   callback);
}

void Vimeo::OnMediaActivityError(uint64_t window_id) {
  const std::string url = TWITTER_TLD;
  const std::string name = TWITTER_MEDIA_TYPE;

  DCHECK(!url.empty());

  ledger::VisitData new_data;
  new_data.domain = url;
  new_data.url = "https://" + url;
  new_data.path = "/";
  new_data.name = name;

  ledger_->GetPublisherActivityFromUrl(window_id,
                                       ledger::VisitData::New(new_data),
                                       "");
}

void Vimeo::ProcessMedia(const std::map<std::string, std::string>& parts) {
  auto iter = parts.find("video_id");
  std::string media_id;
  if (iter != parts.end()) {
    media_id = iter->second;
  }

  if (media_id.empty()) {
    return;
  }

  std::string type;
  iter = parts.find("type");
  if (iter != parts.end()) {
    type = iter->second;
  }

  const std::string media_key = GetMediaKey(media_id, type);

  ledger::MediaEventInfo event_info;
  iter = parts.find("event");
  if (iter != parts.end()) {
    event_info.event_ = iter->second;
  }

  // We should only record events that are relevant to us
  if (!AllowedEvent(event_info.event_)) {
    return;
  }

  iter = parts.find("time");
  if (iter != parts.end()) {
    event_info.time_ = iter->second;
  }

  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&Vimeo::OnMediaPublisherInfo,
                this,
                media_id,
                media_key,
                event_info,
                _1,
                _2));
}

void Vimeo::ProcessActivityFromUrl(uint64_t window_id,
                                   const ledger::VisitData& visit_data) {

}

void Vimeo::OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const ledger::MediaEventInfo& event_info,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Failed to get publisher info";
    return;
  }

  if (!publisher_info && !publisher_info.get()) {
    auto callback = std::bind(&Vimeo::OnPublisherVideoPage,
                            this,
                            media_key,
                            event_info,
                            _1,
                            _2,
                            _3);

    FetchDataFromUrl(GetVideoUrl(media_id), callback);
  } else {
    ledger::MediaEventInfo old_event;
    auto iter = events.find(media_key);
    if (iter != events.end()) {
      old_event = iter->second;
    }

    const uint64_t duration = GetDuration(old_event, event_info);
    events[media_key] = event_info;

    SavePublisherInfo(std::string(),
                      duration,
                      std::string(),
                      publisher_info->name,
                      publisher_info->url,
                      0,
                      publisher_info->id,
                      publisher_info->favicon_url);
  }
}

void Vimeo::OnPublisherVideoPage(
    const std::string& media_key,
    ledger::MediaEventInfo event_info,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError();
    return;
  }

  ledger_->LogResponse(
      __func__,
      response_status_code,
      "HTML from Vimeo Video page",
      headers);

  const std::string user_id = GetIdFromVideoPage(response);

  if (user_id.empty()) {
    return;
  }

  ledger::MediaEventInfo old_event;
  auto iter = events.find(media_key);
  if (iter != events.end()) {
    old_event = iter->second;
  }

  const uint64_t duration = GetDuration(old_event, event_info);
  events[media_key] = event_info;

  SavePublisherInfo(media_key,
                    duration,
                    user_id,
                    GetNameFromVideoPage(response),
                    GetPublisherUrl(response),
                    0);
}

void Vimeo::OnSaveMediaVisit(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
}

void Vimeo::SavePublisherInfo(
    const std::string& media_key,
    const uint64_t duration,
    const std::string& user_id,
    const std::string& publisher_name,
    const std::string& publisher_url,
    const uint64_t window_id,
    const std::string& publisher_key,
    const std::string& publisher_favicon) {

  if (user_id.empty() && publisher_key.empty()) {
    OnMediaActivityError(window_id);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "User id is missing for: " << media_key;
    return;
  }

  auto callback = std::bind(&Vimeo::OnSaveMediaVisit,
                            this,
                            _1,
                            _2);

  std::string key = publisher_key;
  if (key.empty()) {
    key = GetPublisherKey(user_id);
  }

  if (key.empty()) {
    OnMediaActivityError(window_id);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  std::string icon = publisher_favicon;
  if (icon.empty()) {
    icon = GenerateFaviconUrl(user_id);
  }

  ledger::VisitData visit_data;
  visit_data.provider = VIMEO_MEDIA_TYPE;
  visit_data.url = publisher_url;
  visit_data.favicon_url = icon;
  visit_data.name = publisher_name;

  ledger_->SaveMediaVisit(key,
                          visit_data,
                          duration,
                          window_id,
                          callback);

  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, publisher_key);
  }
}

}  // namespace braveledger_media
