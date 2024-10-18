/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/media/youtube.h"

#include <cmath>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/media/helper.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/publisher/static_values.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

namespace {

bool getJSONValue(const std::string& field_name,
                  const std::string& json,
                  std::string* value) {
  auto result =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!result || !result->is_dict()) {
    return false;
  }

  if (auto* field = result->GetDict().FindString(field_name)) {
    *value = *field;
    return true;
  }

  return false;
}

}  // namespace

YouTube::YouTube(RewardsEngine& engine) : engine_(engine) {}

YouTube::~YouTube() = default;

// static
std::string YouTube::GetMediaIdFromParts(
    const base::flat_map<std::string, std::string>& parts) {
  std::string result;
  auto iter = parts.find("docid");
  if (iter != parts.end()) {
    result = iter->second;
  }

  return result;
}

// static
uint64_t YouTube::GetMediaDurationFromParts(
    const base::flat_map<std::string, std::string>& data,
    const std::string& media_key) {
  uint64_t duration = 0;
  auto iter_st = data.find("st");
  auto iter_et = data.find("et");
  if (iter_st != data.end() && iter_et != data.end()) {
    const auto start_time = base::SplitString(
        iter_st->second, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    const auto end_time = base::SplitString(
        iter_et->second, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (start_time.size() != end_time.size()) {
      return 0;
    }

    // get all the intervals and combine them.
    // (Should only be one set if there were no seeks)
    for (size_t i = 0; i < start_time.size(); i++) {
      double st = 0;
      base::StringToDouble(start_time[i], &st);
      double et = 0;
      base::StringToDouble(end_time[i], &et);

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
  std::string favicon_url =
      ExtractData(data, "\"avatar\":{\"thumbnails\":[{\"url\":\"", "\"");

  if (favicon_url.empty()) {
    favicon_url =
        ExtractData(data, "\"width\":88,\"height\":88},{\"url\":\"", "\"");
  }

  return favicon_url;
}

// static
std::string YouTube::GetChannelId(const std::string& data) {
  std::string id = ExtractData(data, "\"ucid\":\"", "\"");
  if (id.empty()) {
    id = ExtractData(data, "HeaderRenderer\":{\"channelId\":\"", "\"");
  }

  if (id.empty()) {
    id = ExtractData(
        data, "<link rel=\"canonical\" href=\"https://www.youtube.com/channel/",
        "\">");
  }

  if (id.empty()) {
    id = ExtractData(data, "browseEndpoint\":{\"browseId\":\"", "\"");
  }

  return id;
}

// static
std::string YouTube::GetPublisherName(const std::string& data) {
  std::string publisher_name;
  std::string publisher_json_name = ExtractData(data, "\"author\":\"", "\"");
  std::string publisher_json =
      "{\"brave_publisher\":\"" + publisher_json_name + "\"}";
  // scraped data could come in with JSON code points added.
  // Make to JSON object above so we can decode.
  getJSONValue("brave_publisher", publisher_json, &publisher_name);
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
std::string YouTube::GetMediaIdFromUrl(const std::string& url) {
  auto first_split = base::SplitString(url, "?", base::TRIM_WHITESPACE,
                                       base::SPLIT_WANT_NONEMPTY);

  if (first_split.size() < 2) {
    return std::string();
  }

  auto and_split = base::SplitString(first_split[1], "&", base::TRIM_WHITESPACE,
                                     base::SPLIT_WANT_NONEMPTY);

  for (const auto& item : and_split) {
    auto m_url = base::SplitString(item, "=", base::TRIM_WHITESPACE,
                                   base::SPLIT_WANT_NONEMPTY);

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
  const std::string publisher_json_name =
      ExtractData(data, "channelMetadataRenderer\":{\"title\":\"", "\"");
  const std::string publisher_json =
      "{\"brave_publisher\":\"" + publisher_json_name + "\"}";
  // scraped data could come in with JSON code points added.
  // Make to JSON object above so we can decode.
  getJSONValue("brave_publisher", publisher_json, &publisher_name);
  return publisher_name;
}

// static
std::string YouTube::GetPublisherKeyFromUrl(const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  const std::string id = ExtractData(path + "/", "/channel/", "/");

  if (id.empty()) {
    return std::string();
  }

  auto params = base::SplitString(id, "?", base::TRIM_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY);

  return params[0];
}

// static
std::string YouTube::GetChannelIdFromCustomPathPage(const std::string& data) {
  return ExtractData(data, "{\"key\":\"browse_id\",\"value\":\"", "\"");
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
  std::vector<std::string> paths(
      {"/feed", "/channel", "/user", "/watch", "/account", "/gaming",
       "/playlist", "/premium", "/reporthistory", "/pair",
       "/account_notifications", "/account_playback", "/account_privacy",
       "/account_sharing", "/account_billing", "/account_advanced",
       "/subscription_manager", "/oops"});

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

  const std::string id = ExtractData(path + "/", "/user/", "/");

  if (id.empty()) {
    return std::string();
  }

  auto params = base::SplitString(id, "?", base::TRIM_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY);

  return params[0];
}

void YouTube::OnMediaActivityError(const mojom::VisitData& visit_data,
                                   uint64_t window_id) {
  std::string url = YOUTUBE_DOMAIN;
  std::string name = YOUTUBE_MEDIA_TYPE;

  if (!url.empty()) {
    mojom::VisitData new_visit_data;
    new_visit_data.domain = url;
    new_visit_data.url = "https://" + url;
    new_visit_data.path = "/";
    new_visit_data.name = name;

    engine_->publisher()->GetPublisherActivityFromUrl(
        window_id, mojom::VisitData::New(new_visit_data), std::string());
  } else {
    engine_->LogError(FROM_HERE) << "Media activity error";
  }
}

void YouTube::ProcessMedia(
    const base::flat_map<std::string, std::string>& parts,
    const mojom::VisitData& visit_data) {
  std::string media_id = GetMediaIdFromParts(parts);
  if (media_id.empty()) {
    return;
  }

  std::string media_key = GetMediaKey(media_id, YOUTUBE_MEDIA_TYPE);
  uint64_t duration = GetMediaDurationFromParts(parts, media_key);

  engine_->database()->GetMediaPublisherInfo(
      media_key,
      base::BindOnce(&YouTube::OnMediaPublisherInfo, weak_factory_.GetWeakPtr(),
                     media_id, media_key, duration, visit_data, 0));
}

void YouTube::ProcessActivityFromUrl(uint64_t window_id,
                                     const mojom::VisitData& visit_data) {
  if (visit_data.path.find("/watch?") != std::string::npos) {
    WatchPath(window_id, visit_data);
    return;
  }

  if (visit_data.path.find("/channel/") != std::string::npos) {
    ChannelPath(window_id, visit_data);
    return;
  }

  if (visit_data.path.find("/user/") != std::string::npos) {
    UserPath(window_id, visit_data);
    return;
  }

  if (!IsPredefinedPath(visit_data.path)) {
    OnPublisherPanleInfo(window_id, visit_data, std::string(), true,
                         mojom::Result::NOT_FOUND, nullptr);
    return;
  }

  OnMediaActivityError(visit_data, window_id);
}

void YouTube::OnMediaPublisherInfo(const std::string& media_id,
                                   const std::string& media_key,
                                   const uint64_t duration,
                                   const mojom::VisitData& visit_data,
                                   const uint64_t window_id,
                                   mojom::Result result,
                                   mojom::PublisherInfoPtr publisher_info) {
  if (result != mojom::Result::OK && result != mojom::Result::NOT_FOUND) {
    engine_->LogError(FROM_HERE) << "Failed to get publisher info";
    return;
  }

  if (!publisher_info) {
    std::string media_url = GetVideoUrl(media_id);

    const std::string url =
        (std::string)YOUTUBE_PROVIDER_URL +
        "?format=json&url=" + base::EscapeQueryParamValue(media_url, false);

    FetchDataFromUrl(
        url,
        base::BindOnce(&YouTube::OnEmbedResponse, weak_factory_.GetWeakPtr(),
                       duration, media_key, media_url, visit_data, window_id));
  } else {
    mojom::VisitData new_visit_data;
    new_visit_data.name = publisher_info->name;
    new_visit_data.url = publisher_info->url;
    new_visit_data.provider = YOUTUBE_MEDIA_TYPE;
    new_visit_data.favicon_url = publisher_info->favicon_url;
    std::string id = publisher_info->id;

    engine_->publisher()->SaveVisit(id, new_visit_data, duration, true,
                                    window_id, base::DoNothing());
  }
}

void YouTube::OnEmbedResponse(const uint64_t duration,
                              const std::string& media_key,
                              const std::string& media_url,
                              const mojom::VisitData& visit_data,
                              const uint64_t window_id,
                              mojom::UrlResponsePtr response) {
  DCHECK(response);
  if (response->status_code != net::HTTP_OK) {
    // embedding disabled, need to scrape
    if (response->status_code == net::HTTP_UNAUTHORIZED) {
      FetchDataFromUrl(
          visit_data.url,
          base::BindOnce(&YouTube::OnPublisherPage, weak_factory_.GetWeakPtr(),
                         duration, media_key, std::string(), std::string(),
                         visit_data, window_id));
    }
    return;
  }

  std::string publisher_url;
  getJSONValue("author_url", response->body, &publisher_url);
  std::string publisher_name;
  getJSONValue("author_name", response->body, &publisher_name);

  FetchDataFromUrl(
      publisher_url,
      base::BindOnce(&YouTube::OnPublisherPage, weak_factory_.GetWeakPtr(),
                     duration, media_key, publisher_url, publisher_name,
                     visit_data, window_id));
}

void YouTube::OnPublisherPage(const uint64_t duration,
                              const std::string& media_key,
                              std::string publisher_url,
                              std::string publisher_name,
                              const mojom::VisitData& visit_data,
                              const uint64_t window_id,
                              mojom::UrlResponsePtr response) {
  DCHECK(response);
  if (response->status_code != net::HTTP_OK && publisher_name.empty()) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (response->status_code == net::HTTP_OK) {
    std::string fav_icon = GetFavIconUrl(response->body);
    std::string channel_id = GetChannelId(response->body);

    if (publisher_name.empty()) {
      publisher_name = GetPublisherName(response->body);
    }

    if (publisher_url.empty()) {
      publisher_url = GetChannelUrl(channel_id);
    }

    SavePublisherInfo(duration, media_key, publisher_url, publisher_name,
                      visit_data, window_id, fav_icon, channel_id);
  }
}

void YouTube::SavePublisherInfo(const uint64_t duration,
                                const std::string& media_key,
                                const std::string& publisher_url,
                                const std::string& publisher_name,
                                const mojom::VisitData& visit_data,
                                const uint64_t window_id,
                                const std::string& fav_icon,
                                const std::string& channel_id) {
  std::string url;
  if (channel_id.empty()) {
    engine_->LogError(FROM_HERE) << "Channel id is missing";
    return;
  }

  std::string publisher_id = GetPublisherKey(channel_id);
  url = publisher_url + "/videos";

  if (publisher_id.empty()) {
    engine_->LogError(FROM_HERE) << "Publisher id is missing";
    return;
  }

  mojom::VisitData new_visit_data;
  if (fav_icon.length() > 0) {
    new_visit_data.favicon_url = fav_icon;
  }

  new_visit_data.provider = YOUTUBE_MEDIA_TYPE;
  new_visit_data.name = publisher_name;
  new_visit_data.url = url;

  engine_->publisher()->SaveVisit(publisher_id, new_visit_data, duration, true,
                                  window_id, base::DoNothing());

  if (!media_key.empty()) {
    engine_->database()->SaveMediaPublisherInfo(media_key, publisher_id,
                                                base::DoNothing());
  }
}

void YouTube::FetchDataFromUrl(const std::string& url,
                               LoadURLCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = url;

  engine_->Get<URLLoader>().Load(std::move(request), URLLoader::LogLevel::kNone,
                                 std::move(callback));
}

void YouTube::WatchPath(uint64_t window_id,
                        const mojom::VisitData& visit_data) {
  std::string media_id = GetMediaIdFromUrl(visit_data.url);
  std::string media_key = GetMediaKey(media_id, YOUTUBE_MEDIA_TYPE);

  if (!media_key.empty() || !media_id.empty()) {
    engine_->database()->GetMediaPublisherInfo(
        media_key, base::BindOnce(&YouTube::OnMediaPublisherActivity,
                                  weak_factory_.GetWeakPtr(), window_id,
                                  visit_data, media_key, media_id));
  } else {
    OnMediaActivityError(visit_data, window_id);
  }
}

void YouTube::OnMediaPublisherActivity(uint64_t window_id,
                                       const mojom::VisitData& visit_data,
                                       const std::string& media_key,
                                       const std::string& media_id,
                                       mojom::Result result,
                                       mojom::PublisherInfoPtr info) {
  if (result != mojom::Result::OK && result != mojom::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (!info || result == mojom::Result::NOT_FOUND) {
    OnMediaPublisherInfo(media_id, media_key, 0, visit_data, window_id, result,
                         std::move(info));
  } else {
    GetPublisherPanleInfo(window_id, visit_data, info->id, false);
  }
}

void YouTube::GetPublisherPanleInfo(uint64_t window_id,
                                    const mojom::VisitData& visit_data,
                                    const std::string& publisher_key,
                                    bool is_custom_path) {
  auto filter = engine_->publisher()->CreateActivityFilter(
      publisher_key, mojom::ExcludeFilter::FILTER_ALL, false,
      engine_->contribution()->GetReconcileStamp(), true, false);
  engine_->database()->GetPanelPublisherInfo(
      std::move(filter),
      base::BindOnce(&YouTube::OnPublisherPanleInfo, weak_factory_.GetWeakPtr(),
                     window_id, visit_data, publisher_key, is_custom_path));
}

void YouTube::OnPublisherPanleInfo(uint64_t window_id,
                                   const mojom::VisitData& visit_data,
                                   const std::string& publisher_key,
                                   bool is_custom_path,
                                   mojom::Result result,
                                   mojom::PublisherInfoPtr info) {
  if (!info || result == mojom::Result::NOT_FOUND) {
    FetchDataFromUrl(visit_data.url,
                     base::BindOnce(&YouTube::GetChannelHeadlineVideo,
                                    weak_factory_.GetWeakPtr(), window_id,
                                    visit_data, is_custom_path));
  } else {
    engine_->client()->OnPanelPublisherInfo(result, std::move(info), window_id);
  }
}

// TODO(nejczdovc): name can be better
void YouTube::GetChannelHeadlineVideo(uint64_t window_id,
                                      const mojom::VisitData& visit_data,
                                      bool is_custom_path,
                                      mojom::UrlResponsePtr response) {
  DCHECK(response);
  if (response->status_code != net::HTTP_OK) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (visit_data.path.find("/channel/") != std::string::npos) {
    std::string title = GetNameFromChannel(response->body);
    std::string favicon = GetFavIconUrl(response->body);
    std::string channel_id = GetPublisherKeyFromUrl(visit_data.path);

    SavePublisherInfo(0, std::string(), visit_data.url, title, visit_data,
                      window_id, favicon, channel_id);

  } else if (is_custom_path) {
    std::string title = GetNameFromChannel(response->body);
    std::string favicon = GetFavIconUrl(response->body);
    std::string channel_id = GetChannelIdFromCustomPathPage(response->body);
    mojom::VisitData new_visit_data;
    new_visit_data.path = "/channel/" + channel_id;
    GetPublisherPanleInfo(window_id, new_visit_data,
                          GetPublisherKey(channel_id), true);
  } else {
    OnMediaActivityError(visit_data, window_id);
  }
}

void YouTube::ChannelPath(uint64_t window_id,
                          const mojom::VisitData& visit_data) {
  std::string key = GetPublisherKeyFromUrl(visit_data.path);
  if (!key.empty()) {
    std::string publisher_key = GetPublisherKey(key);
    GetPublisherPanleInfo(window_id, visit_data, publisher_key, false);
  } else {
    OnMediaActivityError(visit_data, window_id);
  }
}

void YouTube::UserPath(uint64_t window_id, const mojom::VisitData& visit_data) {
  std::string user = GetUserFromUrl(visit_data.path);

  if (user.empty()) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  std::string media_key = (std::string)YOUTUBE_MEDIA_TYPE + "_user_" + user;
  engine_->database()->GetMediaPublisherInfo(
      media_key,
      base::BindOnce(&YouTube::OnUserActivity, weak_factory_.GetWeakPtr(),
                     window_id, visit_data, media_key));
}

void YouTube::OnUserActivity(uint64_t window_id,
                             const mojom::VisitData& visit_data,
                             const std::string& media_key,
                             mojom::Result result,
                             mojom::PublisherInfoPtr info) {
  if (result != mojom::Result::OK && result != mojom::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (!info || result == mojom::Result::NOT_FOUND) {
    FetchDataFromUrl(
        visit_data.url,
        base::BindOnce(&YouTube::OnChannelIdForUser, weak_factory_.GetWeakPtr(),
                       window_id, visit_data, media_key));

  } else {
    GetPublisherPanleInfo(window_id, visit_data, info->id, false);
  }
}

void YouTube::OnChannelIdForUser(uint64_t window_id,
                                 const mojom::VisitData& visit_data,
                                 const std::string& media_key,
                                 mojom::UrlResponsePtr response) {
  DCHECK(response);
  std::string channelId = GetChannelId(response->body);
  if (!channelId.empty()) {
    std::string path = "/channel/" + channelId;
    std::string url = GetChannelUrl(channelId);
    std::string publisher_key = GetPublisherKey(channelId);

    engine_->database()->SaveMediaPublisherInfo(media_key, publisher_key,
                                                base::DoNothing());

    mojom::VisitData new_visit_data;
    new_visit_data.path = path;
    new_visit_data.url = url;
    new_visit_data.name = std::string();
    new_visit_data.favicon_url = std::string();

    ProcessActivityFromUrl(window_id, new_visit_data);
  } else {
    OnMediaActivityError(visit_data, window_id);
  }
}

}  // namespace brave_rewards::internal
