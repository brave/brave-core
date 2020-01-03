/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/vimeo.h"
#include "bat/ledger/internal/static_values.h"
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
    return "";
  }

  return "https://vimeo.com/" + video_id;
}

// static
std::string Vimeo::GetMediaKey(const std::string& video_id,
                               const std::string& type) {
  if (video_id.empty()) {
    return "";
  }

  if (type == "vimeo-vod") {
    return (std::string)VIMEO_MEDIA_TYPE  + "_" + video_id;
  }

  return "";
}

// static
std::string Vimeo::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return "";
  }

  return (std::string)VIMEO_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string Vimeo::GetIdFromVideoPage(const std::string& data) {
  if (data.empty()) {
    return "";
  }

  return braveledger_media::ExtractData(data,
      "\"creator_id\":", ",");
}

// static
std::string Vimeo::GenerateFaviconUrl(const std::string& id) {
  if (id.empty()) {
    return "";
  }

  return base::StringPrintf("https://i.vimeocdn.com/portrait/%s_300x300.webp",
                            id.c_str());
}

// static
std::string Vimeo::GetNameFromVideoPage(const std::string& data) {
  if (data.empty()) {
    return "";
  }

  return braveledger_media::ExtractData(data, ",\"display_name\":\"", "\"");
}

// static
std::string Vimeo::GetUrlFromVideoPage(const std::string& data) {
  if (data.empty()) {
    return "";
  }

  const std::string wrapper = braveledger_media::ExtractData(data,
      "<span class=\"userlink userlink--md\">", "</span>");

  const std::string name = braveledger_media::ExtractData(wrapper,
      "<a href=\"/", "\">");

  if (name.empty()) {
    return "";
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
  if (old_event.event == new_event.event &&
      old_event.time == new_event.time) {
    return 0u;
  }

  double time = 0.0;
  std::stringstream tempNew(new_event.time);
  double newTime = 0.0;
  tempNew >> newTime;

  // Video started
  if (new_event.event == "video-start-time") {
    time = newTime;
  } else {
    std::stringstream tempOld(old_event.time);
    double oldTime = 0;
    tempOld >> oldTime;

    if (new_event.event == "video-minute-watched" ||
        new_event.event == "video-paused") {
      time = newTime - oldTime;
    }
  }

  if (time < 0) {
    time = 0;
  }

  return static_cast<uint64_t>(std::round(time));
}

// static
bool Vimeo::IsExcludedPath(const std::string& path) {
  if (path.empty()) {
    return true;
  }

  const std::vector<std::string> paths({
      "/",
      "/log_in",
      "/upgrade",
      "/live",
      "/watch",
      "/videoschool",
      "/upload",
      "/ondemand",
      "/ott",
      "/site_map",
      "/blog",
      "/help",
      "/about",
      "/jobs",
      "/stats",
      "/watchlater",
      "/purchases",
      "/settings",
      "/stock",
    });

  for (std::string str_path : paths) {
    if (str_path == path || str_path + "/" == path) {
      return true;
    }
  }

  const std::vector<std::string> patterns({
    "/features/",
    "/categories/",
    "/blog/",
    "/ott/",
    "/help/",
    "/manage/",
    "/settings/",
    "/stock/",
  });

  for (std::string str_path : patterns) {
    if (base::StartsWith(path,
                         str_path,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }


  return false;
}

// static
std::string Vimeo::GetIdFromPublisherPage(const std::string& data) {
  if (data.empty()) {
    return "";
  }

  return braveledger_media::ExtractData(
      data,
      "data-deep-link=\"users/",
      "\"");
}

// static
std::string Vimeo::GetNameFromPublisherPage(const std::string& data) {
  if (data.empty()) {
    return "";
  }

  return braveledger_media::ExtractData(data,
      "<meta property=\"og:title\" content=\"", "\"");
}

// static
std::string Vimeo::GetVideoIdFromVideoPage(const std::string& data) {
  if (data.empty()) {
    return "";
  }

  return braveledger_media::ExtractData(
      data,
      "<link rel=\"canonical\" href=\"https://vimeo.com/",
      "\"");
}

void Vimeo::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   "",
                   "",
                   ledger::UrlMethod::GET,
                   callback);
}

void Vimeo::OnMediaActivityError(
    ledger::GetPublisherActivityFromUrlCallback callback) {
  const std::string url = VIMEO_TLD;
  const std::string name = VIMEO_MEDIA_TYPE;

  DCHECK(!url.empty());

  ledger::VisitData new_data;
  new_data.domain = url;
  new_data.url = "https://" + url;
  new_data.path = "/";
  new_data.name = name;

  ledger_->GetPublisherActivityFromUrl(ledger::VisitData::New(new_data),
                                       "",
                                       callback);
}

void Vimeo::ProcessMedia(
    const std::map<std::string, std::string>& parts,
    ledger::GetPublisherActivityFromUrlCallback callback) {
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
    event_info.event = iter->second;
  }

  // We should only record events that are relevant to us
  if (!AllowedEvent(event_info.event)) {
    return;
  }

  iter = parts.find("time");
  if (iter != parts.end()) {
    event_info.time = iter->second;
  }

  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&Vimeo::OnMediaPublisherInfo,
                this,
                media_id,
                media_key,
                event_info,
                callback,
                _1,
                _2));
}

void Vimeo::ProcessActivityFromUrl(
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  // not all url's are publisher specific
  if (IsExcludedPath(visit_data.path)) {
    OnMediaActivityError(callback);
    return;
  }

  const std::string url = (std::string)VIMEO_PROVIDER_URL +
        "?url=" +
        ledger_->URIEncode(visit_data.url);

  auto embed_callback = std::bind(&Vimeo::OnEmbedResponse,
                            this,
                            visit_data,
                            callback,
                            _1,
                            _2,
                            _3);

  FetchDataFromUrl(url, embed_callback);
}

void Vimeo::OnEmbedResponse(
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    auto unknown_page_callback = std::bind(&Vimeo::OnUnknownPage,
                              this,
                              visit_data,
                              callback,
                              _1,
                              _2,
                              _3);

    FetchDataFromUrl(visit_data.url, unknown_page_callback);
    return;
  }

  base::Optional<base::Value> data = base::JSONReader::Read(response);
  if (!data || !data->is_dict()) {
    auto unknown_page_callback = std::bind(&Vimeo::OnUnknownPage,
                              this,
                              visit_data,
                              callback,
                              _1,
                              _2,
                              _3);

    FetchDataFromUrl(visit_data.url, unknown_page_callback);
    return;
  }

  std::string publisher_url;
  if (data->FindStringKey("author_url")) {
    publisher_url = *data->FindStringKey("author_url");
  }

  int32_t video_id = 0;
  if (data->FindIntKey("video_id")) {
    video_id = *data->FindIntKey("video_id");
  }

  if (publisher_url.empty() || video_id == 0) {
    auto unknown_page_callback = std::bind(&Vimeo::OnUnknownPage,
                              this,
                              visit_data,
                              callback,
                              _1,
                              _2,
                              _3);

    FetchDataFromUrl(visit_data.url, unknown_page_callback);
    return;
  }

  std::string publisher_name;
  if (data->FindStringKey("author_name")) {
    publisher_name = *data->FindStringKey("author_name");
  }

  const std::string media_key = GetMediaKey(std::to_string(video_id),
                                            "vimeo-vod");

  auto publisher_page_callback = std::bind(&Vimeo::OnPublisherPage,
                            this,
                            media_key,
                            publisher_url,
                            publisher_name,
                            visit_data,
                            callback,
                            _1,
                            _2,
                            _3);

  FetchDataFromUrl(publisher_url, publisher_page_callback);
}

void Vimeo::OnPublisherPage(
    const std::string& media_key,
    const std::string& publisher_url,
    const std::string& publisher_name,
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(
      __func__,
      response_status_code,
      "HTML from Vimeo publisher page",
      headers);

  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError(callback);
    return;
  }

  const std::string user_id = GetIdFromPublisherPage(response);
  const std::string publisher_key = GetPublisherKey(user_id);

  GetPublisherPanelInfo(media_key,
                        publisher_url,
                        publisher_key,
                        publisher_name,
                        user_id,
                        callback);
}

void Vimeo::OnUnknownPage(
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(
      __func__,
      response_status_code,
      "HTML from Vimeo unknown page",
      headers);

  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError(callback);
    return;
  }

  std::string user_id = GetIdFromPublisherPage(response);
  std::string publisher_name;
  std::string media_key;
  if (!user_id.empty()) {
    // we are on publisher page
    publisher_name = GetNameFromPublisherPage(response);
  } else {
    user_id = GetIdFromVideoPage(response);

    if (user_id.empty()) {
      OnMediaActivityError(callback);
      return;
    }

    // we are on video page
    publisher_name = GetNameFromVideoPage(response);
    media_key = GetMediaKey(GetVideoIdFromVideoPage(response),
                            "vimeo-vod");
  }

  if (publisher_name.empty()) {
    OnMediaActivityError(callback);
    return;
  }

  const std::string publisher_key = GetPublisherKey(user_id);
  GetPublisherPanelInfo(media_key,
                        visit_data.url,
                        publisher_key,
                        publisher_name,
                        user_id,
                        callback);
}

void Vimeo::OnPublisherPanelInfo(
    const std::string& media_key,
    const std::string& publisher_url,
    const std::string& publisher_name,
    const std::string& user_id,
    ledger::GetPublisherActivityFromUrlCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    SavePublisherInfo(media_key,
                      0,
                      user_id,
                      publisher_name,
                      publisher_url,
                      callback);
  } else {
    callback(result, std::move(info));
  }
}

void Vimeo::GetPublisherPanelInfo(
    const std::string& media_key,
    const std::string& publisher_url,
    const std::string& publisher_key,
    const std::string& publisher_name,
    const std::string& user_id,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  auto filter = ledger_->CreateActivityFilter(
    publisher_key,
    ledger::ExcludeFilter::FILTER_ALL,
    false,
    ledger_->GetReconcileStamp(),
    true,
    false);
  ledger_->GetPanelPublisherInfo(std::move(filter),
    std::bind(&Vimeo::OnPublisherPanelInfo,
              this,
              media_key,
              publisher_url,
              publisher_name,
              user_id,
              callback,
              _1,
              _2));
}

void Vimeo::OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const ledger::MediaEventInfo& event_info,
    ledger::GetPublisherActivityFromUrlCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    OnMediaActivityError(callback);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Failed to get publisher info";
    return;
  }

  if (!publisher_info && !publisher_info.get()) {
    auto video_page_callback = std::bind(&Vimeo::OnPublisherVideoPage,
                            this,
                            media_key,
                            event_info,
                            callback,
                            _1,
                            _2,
                            _3);

    FetchDataFromUrl(GetVideoUrl(media_id), video_page_callback);
    return;
  }

  ledger::MediaEventInfo old_event;
  auto iter = events.find(media_key);
  if (iter != events.end()) {
    old_event = iter->second;
  }

  const uint64_t duration = GetDuration(old_event, event_info);
  events[media_key] = event_info;

  SavePublisherInfo("",
                    duration,
                    "",
                    publisher_info->name,
                    publisher_info->url,
                    callback,
                    publisher_info->id,
                    publisher_info->favicon_url);
}

void Vimeo::OnPublisherVideoPage(
    const std::string& media_key,
    ledger::MediaEventInfo event_info,
    ledger::GetPublisherActivityFromUrlCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(
      __func__,
      response_status_code,
      "HTML from Vimeo Video page",
      headers);

  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError(callback);
    return;
  }

  const std::string user_id = GetIdFromVideoPage(response);

  if (user_id.empty()) {
    OnMediaActivityError(callback);
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
                    GetUrlFromVideoPage(response),
                    callback,
                    0);
}

void Vimeo::OnSaveMediaVisit(
    ledger::GetPublisherActivityFromUrlCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  callback(result, std::move(info));
}

void Vimeo::SavePublisherInfo(
    const std::string& media_key,
    const uint64_t duration,
    const std::string& user_id,
    const std::string& publisher_name,
    const std::string& publisher_url,
    ledger::GetPublisherActivityFromUrlCallback callback,
    const std::string& publisher_key,
    const std::string& publisher_favicon) {
  if (user_id.empty() && publisher_key.empty()) {
    OnMediaActivityError(callback);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "User id is missing for: " << media_key;
    return;
  }

  auto save_callback = std::bind(&Vimeo::OnSaveMediaVisit,
                            this,
                            callback,
                            _1,
                            _2);

  std::string key = publisher_key;
  if (key.empty()) {
    key = GetPublisherKey(user_id);
  }

  if (key.empty()) {
    OnMediaActivityError(callback);
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
                          callback);

  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, key);
  }
}

}  // namespace braveledger_media
