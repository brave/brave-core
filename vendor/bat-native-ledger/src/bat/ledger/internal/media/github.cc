/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/github.h"
#include "bat/ledger/internal/static_values.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

GitHub::GitHub(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

GitHub::~GitHub() {
}

// static
std::string GitHub::GetLinkType(const std::string& url) {
  if (url.empty()) {
    return "";
  }

  return url.find(GITHUB_TLD) != std::string::npos ? GITHUB_MEDIA_TYPE : "";
}

// static
bool GitHub::GetJSONIntValue(const std::string& key,
      const std::string& json_string,
      int64_t* result) {
  base::Optional<base::Value> value = base::JSONReader::Read(json_string);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  const auto* item = dictionary->FindKey(key);
  if (item && item->is_int()) {
    *result = item->GetInt();
    return true;
  }
  return false;
}

// static
bool GitHub::GetJSONStringValue(const std::string& key,
      const std::string& json_string,
      std::string* result) {
  base::Optional<base::Value> value = base::JSONReader::Read(json_string);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  const auto* item = dictionary->FindKey(key);
  if (item && item->is_string()) {
    *result = item->GetString();
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
    const ledger::VisitData& visit_data) {
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

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&GitHub::OnMediaPublisherActivity,
                this,
                _1,
                _2,
                window_id,
                visit_data,
                media_key));
}


void GitHub::ProcessMedia(
    const std::map<std::string, std::string> parts,
    const ledger::VisitData& visit_data) {
  const std::string user_name = GetUserNameFromURL(visit_data.path);
  const std::string url = GetProfileAPIURL(user_name);
  auto iter = parts.find("duration");
  uint64_t duration = iter != parts.end() ? std::stoull(iter->second) : 0U;

  if (duration == 0) {
    return;
  }

  const auto callback = std::bind(&GitHub::OnUserPage,
      this,
      duration,
      0,
      visit_data,
      _1,
      _2,
      _3);

  FetchDataFromUrl(url, callback);
}

void GitHub::OnMediaPublisherActivity(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& media_key) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    OnMediaActivityError(window_id);
    return;
  }

  if (!info || result == ledger::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromURL(visit_data.path);
    const std::string url = GetProfileAPIURL(user_name);

    FetchDataFromUrl(url,
                     std::bind(&GitHub::OnUserPage,
                               this,
                               0,
                               window_id,
                               visit_data,
                               _1,
                               _2,
                               _3));
  } else {
    GetPublisherPanelInfo(window_id,
                          visit_data,
                          info->id);
  }
}

void GitHub::OnMediaActivityError(uint64_t window_id) {
  std::string url = GITHUB_TLD;
  std::string name = GITHUB_MEDIA_TYPE;

  DCHECK(!url.empty());

  ledger::VisitData new_visit_data;
  new_visit_data.domain = url;
  new_visit_data.url = "https://" + url;
  new_visit_data.path = "/";
  new_visit_data.name = name;

  ledger_->GetPublisherActivityFromUrl(
      window_id, ledger::VisitData::New(new_visit_data), "");
}

// Gets publisher panel info where we know that publisher info exists
void GitHub::GetPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key) {
  auto filter = ledger_->CreateActivityFilter(
    publisher_key,
    ledger::ExcludeFilter::FILTER_ALL,
    false,
    ledger_->GetReconcileStamp(),
    true,
    false);
  ledger_->GetPanelPublisherInfo(std::move(filter),
    std::bind(&GitHub::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void GitHub::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromURL(visit_data.path);
    const std::string url = GetProfileAPIURL(user_name);
    FetchDataFromUrl(url,
                     std::bind(&GitHub::OnUserPage,
                               this,
                               0,
                               window_id,
                               visit_data,
                               _1,
                               _2,
                               _3));
  } else {
    ledger_->OnPanelPublisherInfo(result, std::move(info), window_id);
  }
}

void GitHub::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   "",
                   "",
                   ledger::UrlMethod::GET,
                   callback);
}

void GitHub::OnUserPage(
    const uint64_t duration,
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError(window_id);
    return;
  }

  const std::string user_id = GetUserId(response);
  const std::string user_name = GetUserNameFromURL(visit_data.path);
  const std::string publisher_name = GetPublisherName(response);
  const std::string profile_picture = GetProfileImageURL(response);

  auto callback = std::bind(&GitHub::OnSaveMediaVisit,
                            this,
                            _1,
                            _2);

  SavePublisherInfo(duration,
                    user_id,
                    user_name,
                    publisher_name,
                    profile_picture,
                    window_id,
                    callback);
}

void GitHub::OnSaveMediaVisit(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
}

void GitHub::SavePublisherInfo(
    const uint64_t duration,
    const std::string& user_id,
    const std::string& screen_name,
    const std::string& publisher_name,
    const std::string& profile_picture,
    const uint64_t window_id,
    ledger::PublisherInfoCallback callback) {
  const std::string publisher_key = GetPublisherKey(user_id);
  const std::string media_key = GetMediaKey(screen_name);

  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  const std::string url = GetProfileURL(screen_name);

  ledger::VisitData visit_data;
  visit_data.provider = GITHUB_MEDIA_TYPE;
  visit_data.url = url;
  visit_data.favicon_url = profile_picture;
  visit_data.name = publisher_name;
  ledger_->SaveMediaVisit(publisher_key,
                          visit_data,
                          duration,
                          window_id,
                          callback);

  if (!media_key.empty()) {
    ledger_->SaveMediaPublisherInfo(
        media_key,
        publisher_key,
        [](const ledger::Result _){});
  }
}

void GitHub::OnMediaPublisherInfo(
    uint64_t window_id,
    const std::string& user_id,
    const std::string& screen_name,
    const std::string& publisher_name,
    const std::string& profile_picture,
    ledger::PublisherInfoCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (result != ledger::Result::LEDGER_OK  &&
    result != ledger::Result::NOT_FOUND) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    SavePublisherInfo(0,
                      user_id,
                      screen_name,
                      publisher_name,
                      profile_picture,
                      window_id,
                      callback);
  } else {
    // TODO(nejczdovc): we need to check if user is verified,
    //  but his image was not saved yet, so that we can fix it
    callback(result, std::move(publisher_info));
  }
}

void GitHub::OnMetaDataGet(
      ledger::PublisherInfoCallback callback,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::TIP_ERROR, nullptr);
    return;
  }

  const std::string user_id = GetUserId(response);
  const std::string user_name = GetUserName(response);
  const std::string media_key = GetMediaKey(user_name);
  const std::string publisher_name = GetPublisherName(response);
  const std::string profile_picture = GetProfileImageURL(response);

  ledger_->GetMediaPublisherInfo(
          media_key,
          std::bind(&GitHub::OnMediaPublisherInfo,
                    this,
                    0,
                    user_id,
                    user_name,
                    publisher_name,
                    profile_picture,
                    callback,
                    _1,
                    _2));
}

void GitHub::SaveMediaInfo(
    const std::map<std::string, std::string>& data,
    ledger::PublisherInfoCallback callback) {
  auto user_name = data.find("user_name");
  std::string url = GetProfileAPIURL(user_name->second);
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   "",
                   "",
                   ledger::UrlMethod::GET,
                   std::bind(&GitHub::OnMetaDataGet,
                              this,
                              std::move(callback),
                              _1,
                              _2,
                              _3));
}
}  // namespace braveledger_media
