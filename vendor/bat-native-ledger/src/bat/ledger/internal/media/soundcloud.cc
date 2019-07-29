/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/optional.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/soundcloud.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

SoundCloud::SoundCloud(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

SoundCloud::~SoundCloud() {
}

// static
std::string SoundCloud::GetUserJSON(const std::string& response) {
  std::string script_body = braveledger_media::ExtractData(
      response,
      "<script>webpackJsonp", "</script>");
  if(script_body.empty()) {
    return "";
  }
  std::string array_str = braveledger_media::ExtractData(
    script_body, "var c=", ",o=Date.now()");

  base::Optional<base::Value> value = base::JSONReader::Read(array_str);

  if(!value || !value->is_list()) {
    return "";
  }
  base::ListValue list = base::ListValue(value->GetList());
  base::DictionaryValue* dictionary = nullptr;
  std::string ret;
  for(auto it = list.begin(); it != list.end(); ++it) {
    it->GetAsDictionary(&dictionary);
    if(!dictionary || !dictionary->is_dict()) {
      continue;
    }
    auto* cur = dictionary->FindKey("id");
    if (cur && cur->is_int() && cur->GetInt() == 64) {
      auto* data = dictionary->FindKey("data");
      if(data && data->is_list()) {
        base::JSONWriter::Write(data->GetList()[0], &ret);
      }
    }
  }
  return ret;
}

// static
bool SoundCloud::GetJSONIntValue(const std::string& key,
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
bool SoundCloud::GetJSONStringValue(const std::string& key,
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
std::string SoundCloud::GetUserName(const std::string& json_string) {
  std::string user_name;
  const bool success = GetJSONStringValue("username", json_string, &user_name);
  return success ?  user_name : "";
}

// static
std::string SoundCloud::GetBaseURL(const std::string& path) {
  if (path.empty()) {
    return "";
  }

  std::vector<std::string> parts = base::SplitString(
      path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.size() > 0) {
    return parts.at(0);
  }

  return "";
}

// static
std::string SoundCloud::GetMediaKey(const std::string& screen_name) {
  if (screen_name.empty()) {
    return "";
  }

  return (std::string)SOUNDCLOUD_MEDIA_TYPE + "_" + screen_name;
}

// static
std::string SoundCloud::GetUserId(const std::string& json_string) {
  int64_t user_id;
  const bool success = GetJSONIntValue("id", json_string, &user_id);
  return success ? std::to_string(user_id) : "";
}

// static
std::string SoundCloud::GetPublisherName(const std::string& json_string) {
  std::string publisher_name;
  const bool success = GetJSONStringValue(
      "full_name",
      json_string,
      &publisher_name);
  return success && !publisher_name.empty() ? publisher_name
                                            : GetUserName(json_string);
}

// static
std::string SoundCloud::GetProfileURL(const std::string& user_url) {
  if (user_url.empty()) {
    return "";
  }

  return base::StringPrintf("https://soundcloud.com/%s", user_url.c_str());
}

// static
std::string SoundCloud::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return "";
  }

  return (std::string)SOUNDCLOUD_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string SoundCloud::GetProfileImageURL(const std::string& json_string) {
  std::string image_url;
  const bool success =
      GetJSONStringValue("avatar_url", json_string, &image_url);
  return success ? image_url : "";
}

// static - might need to add more paths
bool SoundCloud::IsExcludedPath(const std::string& path) {
  if (path.empty()) {
    return true;
  }

  const std::vector<std::string> paths({
      "/",
      "/settings",
      "/explore",
      "/discover",
      "/charts",
      "/notifications",
      "/logout",
      "/search",
      "/stream",
      "/go",
      "/upload",
      "/messages",
      "/you"
    });

  for (std::string str_path : paths) {
    if (str_path == path || str_path + "/" == path) {
      return true;
    }
  }

  return false;
}
void SoundCloud::ProcessActivityFromUrl(uint64_t window_id,
    const ledger::VisitData& visit_data) {
  if (IsExcludedPath(visit_data.path)) {
    OnMediaActivityError(window_id);
    return;
  }

  const std::string user_url = GetBaseURL(visit_data.path);
  const std::string media_key = GetMediaKey(user_url);

  if (media_key.empty()) {
    OnMediaActivityError(window_id);
    return;
  }

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&SoundCloud::OnMediaPublisherActivity,
                this,
                _1,
                _2,
                window_id,
                visit_data,
                media_key));
}

void SoundCloud::OnMediaPublisherActivity(
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
    const std::string user_url = GetBaseURL(visit_data.path);
    const std::string url = GetProfileURL(user_url);
    FetchDataFromUrl(url,
                     std::bind(&SoundCloud::OnUserPage,
                               this,
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

void SoundCloud::OnMediaActivityError(uint64_t window_id) {
  std::string url = SOUNDCLOUD_TLD;
  std::string name = SOUNDCLOUD_MEDIA_TYPE;

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
void SoundCloud::GetPublisherPanelInfo(
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
    std::bind(&SoundCloud::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void SoundCloud::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    const std::string user_name = GetBaseURL(visit_data.path);
    const std::string url = GetProfileURL(user_name);
    FetchDataFromUrl(url,
                     std::bind(&SoundCloud::OnUserPage,
                               this,
                               window_id,
                               visit_data,
                               _1,
                               _2,
                               _3));
  } else {
    ledger_->OnPanelPublisherInfo(result, std::move(info), window_id);
  }
}

void SoundCloud::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   "",
                   "",
                   ledger::URL_METHOD::GET,
                   callback);
}

void SoundCloud::OnUserPage(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError(window_id);
    return;
  }

  std::string data_str = GetUserJSON(response);

  if(data_str.empty()) {
    OnMediaActivityError(window_id);
    return;
  }

  const std::string user_id = GetUserId(data_str);
  const std::string user_url = GetBaseURL(visit_data.path);
  const std::string publisher_name = GetPublisherName(data_str);
  const std::string profile_picture = GetProfileImageURL(data_str);

  auto callback = std::bind(&SoundCloud::OnSaveMediaVisit,
                            this,
                            _1,
                            _2);

  SavePublisherInfo(user_id,
                    user_url,
                    publisher_name,
                    profile_picture,
                    window_id,
                    callback);
}

void SoundCloud::OnSaveMediaVisit(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
}

void SoundCloud::SavePublisherInfo(
    const std::string& user_id,
    const std::string& user_url,
    const std::string& publisher_name,
    const std::string& profile_picture,
    const uint64_t window_id,
    ledger::PublisherInfoCallback callback) {
  const std::string publisher_key = GetPublisherKey(user_id);
  const std::string media_key = GetMediaKey(user_url);

  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  const std::string url = GetProfileURL(user_url);

  ledger::VisitData visit_data;
  visit_data.provider = SOUNDCLOUD_MEDIA_TYPE;
  visit_data.url = url;
  visit_data.favicon_url = profile_picture;
  visit_data.name = publisher_name;

  ledger_->SaveMediaVisit(publisher_key,
                          visit_data,
                          0,
                          window_id,
                          callback);

  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, publisher_key);
  }
}

}  // namespace braveledger_media