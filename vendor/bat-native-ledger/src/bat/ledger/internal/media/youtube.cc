/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <utility>
#include <vector>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/helper.h"
#include "bat/ledger/internal/media/youtube.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

YouTube::YouTube(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

YouTube::~YouTube() {
}

// static
std::string YouTube::GetMediaIdFromParts(
    const std::map<std::string, std::string>& parts) {
  std::string result;
  std::map<std::string, std::string>::const_iterator iter =
        parts.find("docid");
  if (iter != parts.end()) {
    result = iter->second;
  }

  return result;
}

// static
uint64_t YouTube::GetMediaDurationFromParts(
    const std::map<std::string, std::string>& data,
    const std::string& media_key) {
  uint64_t duration = 0;
  std::map<std::string, std::string>::const_iterator iter_st = data.find("st");
  std::map<std::string, std::string>::const_iterator iter_et = data.find("et");
  if (iter_st != data.end() && iter_et != data.end()) {
    std::vector<std::string> start_time = braveledger_bat_helper::split(
        iter_st->second,
        ',');
    std::vector<std::string> end_time = braveledger_bat_helper::split(
        iter_et->second,
        ',');
    if (start_time.size() != end_time.size()) {
      return 0;
    }

    // get all the intervals and combine them.
    // (Should only be one set if there were no seeks)
    for (size_t i = 0; i < start_time.size(); i++) {
      std::stringstream tempET(end_time[i]);
      std::stringstream tempST(start_time[i]);
      double st = 0;
      double et = 0;
      tempET >> et;
      tempST >> st;

      // round instead of truncate
      // also make sure we include previous iterations
      // if more than one set exists
      duration += (uint64_t)std::round(et - st);
    }
  }

  return duration;
}

// static
std::string YouTube::GetVideoUrl(const std::string& media_id) {
  std::string res;
  DCHECK(!media_id.empty());
  return "https://www.youtube.com/watch?v=" + media_id;
}

// static
std::string YouTube::GetChannelUrl(const std::string& publisher_key) {
  std::string res;
  DCHECK(!publisher_key.empty());
  return "https://www.youtube.com/channel/" + publisher_key;
}

// static
std::string YouTube::GetFavIconUrl(const std::string& data) {
  std::string favicon_url = braveledger_media::ExtractData(
      data,
      "\"avatar\":{\"thumbnails\":[{\"url\":\"", "\"");

  if (favicon_url.empty()) {
    favicon_url = braveledger_media::ExtractData(
      data,
      "\"width\":88,\"height\":88},{\"url\":\"", "\"");
  }

  return favicon_url;
}

// static
std::string YouTube::GetChannelId(const std::string& data) {
  std::string id = braveledger_media::ExtractData(data, "\"ucid\":\"", "\"");
  if (id.empty()) {
    id = braveledger_media::ExtractData(
        data,
        "HeaderRenderer\":{\"channelId\":\"", "\"");
  }

  if (id.empty()) {
    id = braveledger_media::ExtractData(
        data,
        "<link rel=\"canonical\" href=\"https://www.youtube.com/channel/",
        "\">");
  }

  if (id.empty()) {
    id = braveledger_media::ExtractData(
      data,
      "browseEndpoint\":{\"browseId\":\"",
      "\"");
  }

  return id;
}

// static
std::string YouTube::GetPublisherName(const std::string& data) {
  std::string publisher_name;
  std::string publisher_json_name = braveledger_media::ExtractData(
      data,
      "\"author\":\"", "\"");
  std::string publisher_json = "{\"brave_publisher\":\"" +
      publisher_json_name + "\"}";
  // scraped data could come in with JSON code points added.
  // Make to JSON object above so we can decode.
  braveledger_bat_helper::getJSONValue(
      "brave_publisher", publisher_json, &publisher_name);
  return publisher_name;
}

// static
std::string YouTube::GetLinkType(const std::string& url) {
  const std::string mobile_api = "https://m.youtube.com/api/stats/watchtime?";
  const std::string desktop_api =
      "https://www.youtube.com/api/stats/watchtime?";
  std::string type;

  if (url.find(mobile_api) != std::string::npos ||
      url.find(desktop_api) != std::string::npos) {
    type = YOUTUBE_MEDIA_TYPE;
  }

  return type;
}

// static
std::string YouTube::GetMediaIdFromUrl(
    const std::string& url) {
  std::vector<std::string> first_split =
    braveledger_bat_helper::split(url, '?');

  if (first_split.size() < 2) {
    return std::string();
  }

  std::vector<std::string> and_split =
    braveledger_bat_helper::split(first_split[1], '&');

  for (const auto& item : and_split) {
    std::vector<std::string> m_url = braveledger_bat_helper::split(item, '=');

    if (m_url.size() < 2) {
      continue;
    }

    if (m_url[0] == "v") {
      return m_url[1];
    }
  }

  return std::string();
}

// static
std::string YouTube::GetNameFromChannel(const std::string& data) {
  std::string publisher_name;
  const std::string publisher_json_name = braveledger_media::ExtractData(data,
      "channelMetadataRenderer\":{\"title\":\"", "\"");
  const std::string publisher_json = "{\"brave_publisher\":\"" +
      publisher_json_name + "\"}";
  // scraped data could come in with JSON code points added.
  // Make to JSON object above so we can decode.
  braveledger_bat_helper::getJSONValue(
      "brave_publisher", publisher_json, &publisher_name);
  return publisher_name;
}

// static
std::string YouTube::GetPublisherKeyFromUrl(
    const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  const std::string id = braveledger_media::ExtractData(path + "/",
      "/channel/", "/");

  if (id.empty()) {
    return std::string();
  }

  std::vector<std::string> params = braveledger_bat_helper::split(id, '?');

  return params[0];
}

// static
std::string YouTube::GetChannelIdFromCustomPathPage(
    const std::string& data) {
  return braveledger_media::ExtractData(data,
      "{\"key\":\"browse_id\",\"value\":\"", "\"");
}

// static
std::string YouTube::GetBasicPath(const std::string& path) {
  std::string yt_path = path.substr(0, path.find("/", 1));
  if (yt_path.empty() || yt_path == path) {
    yt_path = path.substr(0, path.find("?", 1));
    if (yt_path.empty() || yt_path == path) {
      yt_path = path.substr(0);
    }
  }
  return yt_path;
}

// static
bool YouTube::IsPredefinedPath(const std::string& path) {
  std::vector<std::string> paths({
      "/feed",
      "/channel",
      "/user",
      "/watch",
      "/account",
      "/gaming",
      "/playlist",
      "/premium",
      "/reporthistory",
      "/pair",
      "/account_notifications",
      "/account_playback",
      "/account_privacy",
      "/account_sharing",
      "/account_billing",
      "/account_advanced",
      "/subscription_manager",
      "/oops"
    });

  // make sure we are ignoring actual YT paths and not
  // a custom path that might start with a YT path
  const std::string clean_path = GetBasicPath(path);
  for (std::string str_path : paths) {
    if (clean_path == str_path) {
      return true;
    }
  }
  return false;
}

// static
std::string YouTube::GetPublisherKey(const std::string& key) {
  return (std::string)YOUTUBE_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string YouTube::GetUserFromUrl(const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  const std::string id = braveledger_media::ExtractData(path + "/",
      "/user/", "/");

  if (id.empty()) {
    return std::string();
  }

  std::vector<std::string> params = braveledger_bat_helper::split(id, '?');

  return params[0];
}

void YouTube::OnMediaActivityError(
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  std::string url = YOUTUBE_TLD;
  std::string name = YOUTUBE_MEDIA_TYPE;

  if (!url.empty()) {
    ledger::VisitData new_visit_data;
    new_visit_data.domain = url;
    new_visit_data.url = "https://" + url;
    new_visit_data.path = "/";
    new_visit_data.name = name;

    ledger_->GetPublisherActivityFromUrl(
        ledger::VisitData::New(new_visit_data), "", callback);
  } else {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Media activity error for "
        << YOUTUBE_MEDIA_TYPE << " (name: "
        << name << ", url: "
        << visit_data.url << ")";
    callback(ledger::Result::LEDGER_ERROR, nullptr);
  }
}

void YouTube::ProcessMedia(
    const std::map<std::string, std::string>& parts,
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  std::string media_id = GetMediaIdFromParts(parts);
  if (media_id.empty()) {
    return;
  }

  std::string media_key = braveledger_media::GetMediaKey(media_id,
                                                         YOUTUBE_MEDIA_TYPE);
  uint64_t duration = GetMediaDurationFromParts(parts, media_key);

  BLOG(ledger_, ledger::LogLevel::LOG_DEBUG) << "Media key: " << media_key;
  BLOG(ledger_, ledger::LogLevel::LOG_DEBUG) << "Media duration: " << duration;

  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&YouTube::OnMediaPublisherInfo,
                this,
                media_id,
                media_key,
                duration,
                visit_data,
                _1,
                _2,
                callback));
}

void YouTube::ProcessActivityFromUrl(
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  if (visit_data.path.find("/watch?") != std::string::npos) {
    WatchPath(visit_data, callback);
    return;
  }

  if (visit_data.path.find("/channel/") != std::string::npos) {
    ChannelPath(visit_data, callback);
    return;
  }

  if (visit_data.path.find("/user/") != std::string::npos) {
    UserPath(visit_data, callback);
    return;
  }

  if (!IsPredefinedPath(visit_data.path)) {
    OnPublisherPanelInfo(visit_data,
                         "",
                         true,
                         ledger::Result::NOT_FOUND,
                         nullptr,
                         callback);
    return;
  }

  OnMediaActivityError(visit_data, callback);
}

void YouTube::OnSaveMediaVisit(
    ledger::GetPublisherActivityFromUrlCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  callback(result, std::move(info));
}

void YouTube::OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const uint64_t duration,
    const ledger::VisitData& visit_data,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Failed to get publisher info";
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  if (!publisher_info) {
    std::string media_url = GetVideoUrl(media_id);
    auto embed_callback = std::bind(
        &YouTube::OnEmbedResponse,
        this,
        duration,
        media_key,
        media_url,
        visit_data,
        callback,
        _1,
        _2,
        _3);

    const std::string url = (std::string)YOUTUBE_PROVIDER_URL +
        "?format=json&url=" +
        ledger_->URIEncode(media_url);

    FetchDataFromUrl(url, embed_callback);
  } else {
    ledger::VisitData new_visit_data;
    new_visit_data.name = publisher_info->name;
    new_visit_data.url = publisher_info->url;
    new_visit_data.provider = YOUTUBE_MEDIA_TYPE;
    new_visit_data.favicon_url = publisher_info->favicon_url;
    std::string id = publisher_info->id;

    auto save_media_callback = std::bind(&YouTube::OnSaveMediaVisit,
                              this,
                              callback,
                              _1,
                              _2);

    ledger_->SaveMediaVisit(id,
                            new_visit_data,
                            duration,
                            save_media_callback);
  }
}

void YouTube::OnEmbedResponse(
    const uint64_t duration,
    const std::string& media_key,
    const std::string& media_url,
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    // embedding disabled, need to scrape
    if (response_status_code == net::HTTP_UNAUTHORIZED) {
      FetchDataFromUrl(visit_data.url,
          std::bind(&YouTube::OnPublisherPage,
                    this,
                    duration,
                    media_key,
                    "",
                    "",
                    visit_data,
                    callback,
                    _1,
                    _2,
                    _3));
    }
    return;
  }

  std::string publisher_url;
  braveledger_bat_helper::getJSONValue("author_url", response, &publisher_url);
  std::string publisher_name;
  braveledger_bat_helper::getJSONValue("author_name",
                                       response,
                                       &publisher_name);

  auto fetch_callback = std::bind(&YouTube::OnPublisherPage,
                            this,
                            duration,
                            media_key,
                            publisher_url,
                            publisher_name,
                            visit_data,
                            callback,
                            _1,
                            _2,
                            _3);

  FetchDataFromUrl(publisher_url, fetch_callback);
}

void YouTube::OnPublisherPage(
    const uint64_t duration,
    const std::string& media_key,
    std::string publisher_url,
    std::string publisher_name,
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK && publisher_name.empty()) {
    OnMediaActivityError(visit_data, callback);
    return;
  }

  if (response_status_code == net::HTTP_OK) {
    std::string fav_icon = GetFavIconUrl(response);
    std::string channel_id = GetChannelId(response);

    if (publisher_name.empty()) {
      publisher_name = GetPublisherName(response);
    }

    if (publisher_url.empty()) {
      publisher_url = GetChannelUrl(channel_id);
    }

    SavePublisherInfo(duration,
                      media_key,
                      publisher_url,
                      publisher_name,
                      visit_data,
                      callback,
                      fav_icon,
                      channel_id);
  }
}

void YouTube::SavePublisherInfo(
    const uint64_t duration,
    const std::string& media_key,
    const std::string& publisher_url,
    const std::string& publisher_name,
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback,
    const std::string& fav_icon,
    const std::string& channel_id) {
  std::string url;
  if (channel_id.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Channel id is missing for: " << media_key;
      callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  std::string publisher_id = GetPublisherKey(channel_id);
  url = publisher_url + "/videos";

  if (publisher_id.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher id is missing for: " << media_key;
      callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  ledger::VisitData new_visit_data;
  if (fav_icon.length() > 0) {
    new_visit_data.favicon_url = fav_icon;
  }

  new_visit_data.provider = YOUTUBE_MEDIA_TYPE;
  new_visit_data.name = publisher_name;
  new_visit_data.url = url;

  auto visit_callback = std::bind(&YouTube::OnSaveMediaVisit,
                            this,
                            callback,
                            _1,
                            _2);

  ledger_->SaveMediaVisit(publisher_id,
                          new_visit_data,
                          duration,
                          visit_callback);
  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, publisher_id);
  }
}

void YouTube::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   std::string(),
                   std::string(),
                   ledger::UrlMethod::GET,
                   callback);
}

void YouTube::WatchPath(const ledger::VisitData& visit_data,
                        ledger::GetPublisherActivityFromUrlCallback callback) {
  std::string media_id = GetMediaIdFromUrl(visit_data.url);
  std::string media_key = braveledger_media::GetMediaKey(media_id,
                                                         YOUTUBE_MEDIA_TYPE);

  if (!media_key.empty() || !media_id.empty()) {
    ledger_->GetMediaPublisherInfo(
        media_key,
        std::bind(&YouTube::OnMediaPublisherActivity,
                  this,
                  _1,
                  _2,
                  visit_data,
                  media_key,
                  media_id,
                  callback));
  } else {
    OnMediaActivityError(visit_data, callback);
  }
}

void YouTube::OnMediaPublisherActivity(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    const ledger::VisitData& visit_data,
    const std::string& media_key,
    const std::string& media_id,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, callback);
    return;
  }

  if (!info || result == ledger::Result::NOT_FOUND) {
    OnMediaPublisherInfo(media_id,
                         media_key,
                         0,
                         visit_data,
                         result,
                         std::move(info),
                         callback);
  } else {
    GetPublisherPanleInfo(visit_data,
                          info->id,
                          false,
                          callback);
  }
}

void YouTube::GetPublisherPanleInfo(
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    bool is_custom_path,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  auto filter = ledger_->CreateActivityFilter(
    publisher_key,
    ledger::ExcludeFilter::FILTER_ALL,
    false,
    ledger_->GetReconcileStamp(),
    true,
    false);
  ledger_->GetPanelPublisherInfo(std::move(filter),
    std::bind(&YouTube::OnPublisherPanelInfo,
              this,
              visit_data,
              publisher_key,
              is_custom_path,
              _1,
              _2,
              callback));
}

void YouTube::OnPublisherPanelInfo(
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    bool is_custom_path,
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(visit_data.url,
                     std::bind(&YouTube::GetChannelHeadlineVideo,
                               this,
                               visit_data,
                               is_custom_path,
                               callback,
                               _1,
                               _2,
                               _3));
  } else {
    callback(result, std::move(info));
  }
}

// TODO(nejczdovc): name can be better
void YouTube::GetChannelHeadlineVideo(
    const ledger::VisitData& visit_data,
    bool is_custom_path,
    ledger::GetPublisherActivityFromUrlCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError(visit_data, callback);
    return;
  }

  if (visit_data.path.find("/channel/") != std::string::npos) {
    std::string title = GetNameFromChannel(response);
    std::string favicon = GetFavIconUrl(response);
    std::string channel_id = GetPublisherKeyFromUrl(visit_data.path);

    SavePublisherInfo(0,
                      std::string(),
                      visit_data.url,
                      title,
                      visit_data,
                      callback,
                      favicon,
                      channel_id);

  } else if (is_custom_path) {
    std::string title = GetNameFromChannel(response);
    std::string favicon = GetFavIconUrl(response);
    std::string channel_id = GetChannelIdFromCustomPathPage(response);
    ledger::VisitData new_visit_data;
    new_visit_data.path = "/channel/" + channel_id;
    GetPublisherPanleInfo(new_visit_data,
                          GetPublisherKey(channel_id),
                          true,
                          callback);
  } else {
    OnMediaActivityError(visit_data, callback);
  }
}

void YouTube::ChannelPath(
    const ledger::VisitData& visit_data,
    ledger::GetPublisherActivityFromUrlCallback callback) {
  std::string key = GetPublisherKeyFromUrl(visit_data.path);
  if (!key.empty()) {
    std::string publisher_key = GetPublisherKey(key);
    GetPublisherPanleInfo(visit_data,
                          publisher_key,
                          false,
                          callback);
  } else {
    OnMediaActivityError(visit_data, callback);
  }
}

void YouTube::UserPath(const ledger::VisitData& visit_data,
                       ledger::GetPublisherActivityFromUrlCallback callback) {
  std::string user = GetUserFromUrl(visit_data.path);

  if (user.empty()) {
    OnMediaActivityError(visit_data, callback);
    return;
  }

  std::string media_key = (std::string)YOUTUBE_MEDIA_TYPE + "_user_" + user;
  ledger_->GetMediaPublisherInfo(media_key,
                                 std::bind(&YouTube::OnUserActivity,
                                           this,
                                           visit_data,
                                           media_key,
                                           callback,
                                           _1,
                                           _2));
}

void YouTube::OnUserActivity(
    const ledger::VisitData& visit_data,
    const std::string& media_key,
    ledger::GetPublisherActivityFromUrlCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (result != ledger::Result::LEDGER_OK  &&
      result != ledger::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, callback);
    return;
  }

  if (!info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(visit_data.url,
                     std::bind(&YouTube::OnChannelIdForUser,
                               this,
                               visit_data,
                               media_key,
                               callback,
                               _1,
                               _2,
                               _3));

  } else {
    GetPublisherPanleInfo(visit_data,
                          info->id,
                          false,
                          callback);
  }
}

void YouTube::OnChannelIdForUser(
    const ledger::VisitData& visit_data,
    const std::string& media_key,
    ledger::GetPublisherActivityFromUrlCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  std::string channelId = GetChannelId(response);
  if (!channelId.empty()) {
    std::string path = "/channel/" + channelId;
    std::string url = GetChannelUrl(channelId);
    std::string publisher_key = GetPublisherKey(channelId);

    ledger_->SetMediaPublisherInfo(media_key, publisher_key);

    ledger::VisitData new_visit_data;
    new_visit_data.path = path;
    new_visit_data.url = url;
    new_visit_data.name = std::string();
    new_visit_data.favicon_url = std::string();

    ProcessActivityFromUrl(new_visit_data, callback);
  } else {
    OnMediaActivityError(visit_data, callback);
  }
}


}  // namespace braveledger_media
