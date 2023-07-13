/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/legacy/media/github.h"
#include "brave/components/brave_rewards/core/legacy/static_values.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace brave_rewards::internal {

GitHub::GitHub(RewardsEngineImpl& engine) : engine_(engine) {}

GitHub::~GitHub() = default;

// static
std::string GitHub::GetLinkType(const std::string& url) {
  if (url.empty()) {
    return "";
  }

  return url.find(GITHUB_DOMAIN) != std::string::npos ? GITHUB_MEDIA_TYPE : "";
}

// static
bool GitHub::GetJSONIntValue(const std::string& key,
                             const std::string& json_string,
                             int64_t* result) {
  absl::optional<base::Value> value = base::JSONReader::Read(json_string);
  if (!value || !value->is_dict()) {
    return false;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto item = dict.FindInt(key);
  if (item) {
    *result = *item;
    return true;
  }
  return false;
}

// static
bool GitHub::GetJSONStringValue(const std::string& key,
                                const std::string& json_string,
                                std::string* result) {
  absl::optional<base::Value> value = base::JSONReader::Read(json_string);
  if (!value || !value->is_dict()) {
    return false;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* item = dict.FindString(key);
  if (item) {
    *result = *item;
    return true;
  }
  return false;
}

// static
std::string GitHub::GetUserNameFromURL(const std::string& path) {
  if (path.empty()) {
    return "";
  }

  std::vector<std::string> parts = base::SplitString(
      path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.size() > 0) {
    if (parts.size() > 1 && parts.at(0) == "orgs") {
      return parts.at(1);
    }
    return parts.at(0);
  }

  return "";
}

// static
std::string GitHub::GetUserName(const std::string& json_string) {
  std::string publisher_name;
  bool success = GetJSONStringValue("login", json_string, &publisher_name);
  return success ? publisher_name : "";
}

// static
std::string GitHub::GetMediaKey(const std::string& screen_name) {
  if (screen_name.empty()) {
    return "";
  }

  return (std::string)GITHUB_MEDIA_TYPE + "_" + screen_name;
}

// static
std::string GitHub::GetUserId(const std::string& json_string) {
  int64_t user_id;
  const bool success = GetJSONIntValue("id", json_string, &user_id);
  return success ? std::to_string(user_id) : "";
}

// static
std::string GitHub::GetPublisherName(const std::string& json_string) {
  std::string publisher_name = "";
  bool success = GetJSONStringValue("name", json_string, &publisher_name);
  if (success) {
    return publisher_name.empty() ? GetUserName(json_string) : publisher_name;
  }
  return GetUserName(json_string);
}

// static
std::string GitHub::GetProfileURL(const std::string& screen_name) {
  if (screen_name.empty()) {
    return "";
  }

  return base::StringPrintf("https://github.com/%s", screen_name.c_str());
}
// static
std::string GitHub::GetProfileAPIURL(const std::string& screen_name) {
  if (screen_name.empty()) {
    return "";
  }

  return base::StringPrintf("https://api.github.com/users/%s",
                            screen_name.c_str());
}

// static
std::string GitHub::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return "";
  }

  return (std::string)GITHUB_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string GitHub::GetProfileImageURL(const std::string& json_string) {
  std::string image_url;
  const bool success =
      GetJSONStringValue("avatar_url", json_string, &image_url);
  return success ? image_url : "";
}

// static - might need to add more paths
bool GitHub::IsExcludedPath(const std::string& path) {
  if (path.empty()) {
    return true;
  }

  const std::vector<std::string> paths({
      "/",
      "/settings",
      "/explore",
      "/notifications",
      "/logout",
      "/search",
      "/about",
      "/tos",
      "/home",
      "/marketplace",
      "/explore",
      "/issues",
      "/pulls",
  });

  for (std::string str_path : paths) {
    if (str_path == path || str_path + "/" == path) {
      return true;
    }
  }

  return false;
}
void GitHub::ProcessActivityFromUrl(uint64_t window_id,
                                    const mojom::VisitData& visit_data) {
  if (IsExcludedPath(visit_data.path)) {
    OnMediaActivityError(window_id);
    return;
  }

  const std::string user_name = GetUserNameFromURL(visit_data.path);
  const std::string media_key = GetMediaKey(user_name);

  if (media_key.empty()) {
    OnMediaActivityError(window_id);
    return;
  }

  engine_->database()->GetMediaPublisherInfo(
      media_key, std::bind(&GitHub::OnMediaPublisherActivity, this, _1, _2,
                           window_id, visit_data, media_key));
}

void GitHub::ProcessMedia(const base::flat_map<std::string, std::string> parts,
                          const mojom::VisitData& visit_data) {
  const std::string user_name = GetUserNameFromURL(visit_data.path);
  const std::string url = GetProfileAPIURL(user_name);
  auto iter = parts.find("duration");
  uint64_t duration = iter != parts.end() ? std::stoull(iter->second) : 0U;

  if (duration == 0) {
    return;
  }

  const auto callback =
      std::bind(&GitHub::OnUserPage, this, duration, 0, visit_data, _1);

  FetchDataFromUrl(url, callback);
}

void GitHub::OnMediaPublisherActivity(mojom::Result result,
                                      mojom::PublisherInfoPtr info,
                                      uint64_t window_id,
                                      const mojom::VisitData& visit_data,
                                      const std::string& media_key) {
  if (result != mojom::Result::OK && result != mojom::Result::NOT_FOUND) {
    OnMediaActivityError(window_id);
    return;
  }

  if (!info || result == mojom::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromURL(visit_data.path);
    const std::string url = GetProfileAPIURL(user_name);

    auto url_callback =
        std::bind(&GitHub::OnUserPage, this, 0, window_id, visit_data, _1);

    FetchDataFromUrl(url, url_callback);
  } else {
    GetPublisherPanelInfo(window_id, visit_data, info->id);
  }
}

void GitHub::OnMediaActivityError(uint64_t window_id) {
  std::string url = GITHUB_DOMAIN;
  std::string name = GITHUB_MEDIA_TYPE;

  DCHECK(!url.empty());

  mojom::VisitData new_visit_data;
  new_visit_data.domain = url;
  new_visit_data.url = "https://" + url;
  new_visit_data.path = "/";
  new_visit_data.name = name;

  engine_->publisher()->GetPublisherActivityFromUrl(
      window_id, mojom::VisitData::New(new_visit_data), "");
}

// Gets publisher panel info where we know that publisher info exists
void GitHub::GetPublisherPanelInfo(uint64_t window_id,
                                   const mojom::VisitData& visit_data,
                                   const std::string& publisher_key) {
  auto filter = engine_->publisher()->CreateActivityFilter(
      publisher_key, mojom::ExcludeFilter::FILTER_ALL, false,
      engine_->state()->GetReconcileStamp(), true, false);
  engine_->database()->GetPanelPublisherInfo(
      std::move(filter),
      std::bind(&GitHub::OnPublisherPanelInfo, this, window_id, visit_data,
                publisher_key, _1, _2));
}

void GitHub::OnPublisherPanelInfo(uint64_t window_id,
                                  const mojom::VisitData& visit_data,
                                  const std::string& publisher_key,
                                  mojom::Result result,
                                  mojom::PublisherInfoPtr info) {
  if (!info || result == mojom::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromURL(visit_data.path);
    const std::string url = GetProfileAPIURL(user_name);

    auto url_callback =
        std::bind(&GitHub::OnUserPage, this, 0, window_id, visit_data, _1);
    FetchDataFromUrl(url, url_callback);
  } else {
    engine_->client()->OnPanelPublisherInfo(result, std::move(info), window_id);
  }
}

void GitHub::FetchDataFromUrl(const std::string& url,
                              LegacyLoadURLCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = url;
  request->skip_log = true;
  engine_->LoadURL(std::move(request), callback);
}

void GitHub::OnUserPage(const uint64_t duration,
                        uint64_t window_id,
                        const mojom::VisitData& visit_data,
                        mojom::UrlResponsePtr response) {
  DCHECK(response);
  if (response->status_code != net::HTTP_OK) {
    OnMediaActivityError(window_id);
    return;
  }

  const std::string user_id = GetUserId(response->body);
  const std::string user_name = GetUserNameFromURL(visit_data.path);
  const std::string publisher_name = GetPublisherName(response->body);
  const std::string profile_picture = GetProfileImageURL(response->body);

  SavePublisherInfo(duration, user_id, user_name, publisher_name,
                    profile_picture, window_id,
                    [](mojom::Result, mojom::PublisherInfoPtr) {});
}

void GitHub::SavePublisherInfo(const uint64_t duration,
                               const std::string& user_id,
                               const std::string& screen_name,
                               const std::string& publisher_name,
                               const std::string& profile_picture,
                               const uint64_t window_id,
                               PublisherInfoCallback callback) {
  const std::string publisher_key = GetPublisherKey(user_id);
  const std::string media_key = GetMediaKey(screen_name);

  if (publisher_key.empty()) {
    callback(mojom::Result::FAILED, nullptr);
    BLOG(0, "Publisher key is missing");
    return;
  }

  const std::string url = GetProfileURL(screen_name);

  mojom::VisitData visit_data;
  visit_data.provider = GITHUB_MEDIA_TYPE;
  visit_data.url = url;
  visit_data.favicon_url = profile_picture;
  visit_data.name = publisher_name;

  engine_->publisher()->SaveVisit(publisher_key, visit_data, duration, true,
                                  window_id, callback);

  if (!media_key.empty()) {
    engine_->database()->SaveMediaPublisherInfo(media_key, publisher_key,
                                                [](const mojom::Result) {});
  }
}

void GitHub::OnMediaPublisherInfo(uint64_t window_id,
                                  const std::string& user_id,
                                  const std::string& screen_name,
                                  const std::string& publisher_name,
                                  const std::string& profile_picture,
                                  PublisherInfoCallback callback,
                                  mojom::Result result,
                                  mojom::PublisherInfoPtr publisher_info) {
  if (result != mojom::Result::OK && result != mojom::Result::NOT_FOUND) {
    callback(mojom::Result::FAILED, nullptr);
    return;
  }

  if (!publisher_info || result == mojom::Result::NOT_FOUND) {
    SavePublisherInfo(0, user_id, screen_name, publisher_name, profile_picture,
                      window_id, callback);
  } else {
    // TODO(nejczdovc): we need to check if user is verified,
    //  but his image was not saved yet, so that we can fix it
    callback(result, std::move(publisher_info));
  }
}

void GitHub::OnMetaDataGet(PublisherInfoCallback callback,
                           mojom::UrlResponsePtr response) {
  DCHECK(response);
  if (response->status_code != net::HTTP_OK) {
    callback(mojom::Result::TIP_ERROR, nullptr);
    return;
  }

  const std::string user_id = GetUserId(response->body);
  const std::string user_name = GetUserName(response->body);
  const std::string media_key = GetMediaKey(user_name);
  const std::string publisher_name = GetPublisherName(response->body);
  const std::string profile_picture = GetProfileImageURL(response->body);

  engine_->database()->GetMediaPublisherInfo(
      media_key,
      std::bind(&GitHub::OnMediaPublisherInfo, this, 0, user_id, user_name,
                publisher_name, profile_picture, callback, _1, _2));
}

void GitHub::SaveMediaInfo(const base::flat_map<std::string, std::string>& data,
                           PublisherInfoCallback callback) {
  auto user_name = data.find("user_name");
  std::string url = GetProfileAPIURL(user_name->second);

  auto url_callback =
      std::bind(&GitHub::OnMetaDataGet, this, std::move(callback), _1);

  auto request = mojom::UrlRequest::New();
  request->url = url;
  request->skip_log = true;
  engine_->LoadURL(std::move(request), url_callback);
}
}  // namespace brave_rewards::internal
