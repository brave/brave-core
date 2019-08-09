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
#include "bat/ledger/internal/media/stackoverflow.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

StackOverflow::StackOverflow(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

StackOverflow::~StackOverflow() {
}

// static
bool StackOverflow::GetJSONIntValue(const std::string& key,
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
bool StackOverflow::GetJSONStringValue(const std::string& key,
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
std::string StackOverflow::GetIdFromURL(const std::string& path) {
  if(path.empty()) {
      return "";
  }

  std::vector<std::string> parts = base::SplitString(
      path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if(parts.size() < 2) {
      return "";
  }

  return parts.at(1);
}

// static
std::string StackOverflow::GetAPIURLForPostId(const std::string&  post_id) {
    if (post_id.empty()) {
        return "";
    }
    return base::StringPrintf(
        "https://api.stackexchange.com/2.2/posts/%s?order=desc&sort=activity&site=stackoverflow", // NO LINT
        post_id.c_str());
}

// static
std::string StackOverflow::GetAPIURLForUserId(const std::string&  user_id) {
    if (user_id.empty()) {
        return "";
    }
    return base::StringPrintf(
        "https://api.stackexchange.com/2.2/users/%s?order=desc&sort=reputation&site=stackoverflow", // NO LINT
        user_id.c_str());
}

// static
std::string StackOverflow::GetUserNameFromURL(const std::string& path) {
  if (path.empty()) {
    return "";
  }

  std::vector<std::string> parts = base::SplitString(
      path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (parts.size() > 0) {
     return parts.at(parts.size() - 1);
  }

  return "";
}

// static
std::string StackOverflow::GetUserName(const std::string& json_string) {
  std::string publisher_name;
  bool success = GetJSONStringValue("login", json_string, &publisher_name);
  return success ? publisher_name : "";
}

// static
std::string StackOverflow::GetMediaKey(const std::string& screen_name) {
  if (screen_name.empty()) {
    return "";
  }

  return (std::string)STACKOVERFLOW_MEDIA_TYPE + "_" + screen_name;
}

// static
std::string StackOverflow::GetUserId(const std::string& json_string) {
  int64_t user_id;
  const bool success = GetJSONIntValue("id", json_string, &user_id);
  return success ? std::to_string(user_id) : "";
}

// static
std::string StackOverflow::GetPublisherName(const std::string& json_string) {
  std::string publisher_name = "";
  bool success = GetJSONStringValue("name", json_string, &publisher_name);
  if (success) {
    return publisher_name.empty() ? GetUserName(json_string) : publisher_name;
  }
  return GetUserName(json_string);
}

// static
std::string StackOverflow::GetProfileURL(const std::string& screen_name) {
  if (screen_name.empty()) {
    return "";
  }

  return base::StringPrintf("https://stackoverflow.com/%s", screen_name.c_str());
}
// static
std::string StackOverflow::GetProfileAPIURL(const std::string& screen_name) {
  if (screen_name.empty()) {
    return "";
  }

  return base::StringPrintf("https://api.stackoverflow.com/users/%s",
      screen_name.c_str());
}

// static
std::string StackOverflow::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return "";
  }

  return (std::string)STACKOVERFLOW_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string StackOverflow::GetProfileImageURL(const std::string& json_string) {
  std::string image_url;
  const bool success =
      GetJSONStringValue("avatar_url", json_string, &image_url);
  return success ? image_url : "";
}

// static - might need to add more paths
bool StackOverflow::IsExcludedPath(const std::string& path) {
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

void StackOverflow::ProcessActivityFromUrl(uint64_t window_id,
    const ledger::VisitData& visit_data) {
  if (visit_data.path.find("/questions/") != std::string::npos) {
      PostPath(window_id, visit_data);
      return;
  } else if (visit_data.path.find("/users/") != std::string::npos) {
      UserPath(window_id, visit_data);
      return;
  }

  OnMediaActivityError(window_id);
}

void StackOverflow::UserPath(uint64_t window_id,
    const ledger::VisitData& visit_data) {
  std::string post_id = GetIdFromURL(visit_data.path);
  std::string url = GetAPIURLForUserId(post_id);

  if (url.empty()) {
      OnMediaActivityError(window_id);
      return;
  }

  FetchDataFromUrl(url,
      std::bind(&StackOverflow::OnUserPath,
          this,
          window_id,
          visit_data,
          _1,
          _2,
          _3));
}

void StackOverflow::PostPath(uint64_t window_id,
    const ledger::VisitData& visit_data) {
  std::string post_id = GetIdFromURL(visit_data.path);
  std::string url = GetAPIURLForPostId(post_id);

  if (url.empty()) {
      OnMediaActivityError(window_id);
      return;
  }

  FetchDataFromUrl(url,
      std::bind(&StackOverflow::OnPostPath,
          this,
          window_id,
          visit_data,
          _1,
          _2,
          _3));
}

void StackOverflow::OnUserPath(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  auto response_dict = base::JSONReader::Read(response);
  if (!response_dict || !response_dict->is_dict()) {
      OnMediaActivityError(window_id);
  }

  auto* items = response_dict->FindKey("items");
  if (!items || !items->is_list()) {
      OnMediaActivityError(window_id);
  }

  auto& items_list = items->GetList();
  if (!items_list.size()) {
      OnMediaActivityError(window_id);
  }

  auto& body = items_list[0];
  auto* user_id_val = body.FindKey("user_id");
  auto* display_name_val = body.FindKey("display_name");
  auto* profile_url_val = body.FindKey("link");
  auto* profile_image_val = body.FindKey("profile_image");

  if (!(user_id_val &&
      display_name_val &&
      profile_url_val &&
      profile_image_val)) {
    OnMediaActivityError(window_id);
  }

  if (!(user_id_val->is_int() &&
      display_name_val->is_string() &&
      profile_url_val->is_string()) &&
      profile_image_val->is_string()) {
    OnMediaActivityError(window_id);
  }

  int32_t user_id = user_id_val->GetInt();
  std::string display_name = display_name_val->GetString();
  std::string profile_url = profile_url_val->GetString();
  std::string user_name = GetUserNameFromURL(profile_url);
  std::string profile_image = profile_image_val->GetString();
  std::string media_key = GetMediaKey(user_name);

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&StackOverflow::OnMediaPublisherActivity,
          this,
          _1,
          _2,
          window_id,
          visit_data,
          media_key,
          std::to_string(user_id),
          display_name,
          profile_url,
          profile_image));
}

void StackOverflow::OnPostPath(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  auto response_dict = base::JSONReader::Read(response);
  if (!response_dict || !response_dict->is_dict()) {
      OnMediaActivityError(window_id);
  }

  auto* items = response_dict->FindKey("items");
  if (!items || !items->is_list()) {
      OnMediaActivityError(window_id);
  }

  auto& items_list = items->GetList();
  if (!items_list.size()) {
      OnMediaActivityError(window_id);
  }

  auto& body = items_list[0];
  auto* owner = body.FindKey("owner");
  if (!owner || !owner->is_dict()) {
      OnMediaActivityError(window_id);
  }

  auto* user_id_val = owner->FindKey("user_id");
  auto* display_name_val = owner->FindKey("display_name");
  auto* profile_url_val = owner->FindKey("link");
  auto* profile_image_val = owner->FindKey("profile_image");

  if (!(user_id_val &&
      display_name_val &&
      profile_url_val &&
      profile_image_val)) {
    OnMediaActivityError(window_id);
  }

  if (!(user_id_val->is_int() &&
      display_name_val->is_string() &&
      profile_url_val->is_string()) &&
      profile_image_val->is_string()) {
    OnMediaActivityError(window_id);
  }

  int32_t user_id = user_id_val->GetInt();
  std::string display_name = display_name_val->GetString();
  std::string profile_url = profile_url_val->GetString();
  std::string user_name = GetUserNameFromURL(profile_url);
  std::string profile_image = profile_image_val->GetString();
  std::string media_key = GetMediaKey(user_name);

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&StackOverflow::OnMediaPublisherActivity,
          this,
          _1,
          _2,
          window_id,
          visit_data,
          media_key,
          std::to_string(user_id),
          display_name,
          profile_url,
          profile_image));
}

void StackOverflow::OnMediaPublisherActivity(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& media_key,
    const std::string& user_id,
    const std::string& publisher_name,
    const std::string& profile_url,
    const std::string& profile_image) {

  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    OnMediaActivityError(window_id);
    return;
  }

  if (!info || result == ledger::Result::NOT_FOUND) {
    auto callback = std::bind(&StackOverflow::OnSaveMediaVisit,
                              this,
                              _1,
                              _2);
    SavePublisherInfo(
        media_key,
        user_id,
        publisher_name,
        profile_url,
        profile_image,
        window_id,
        callback);
  } else {
    GetPublisherPanelInfo(window_id,
                          visit_data,
                          info->id);
  }
}

void StackOverflow::OnMediaActivityError(uint64_t window_id) {
  std::string url = STACKOVERFLOW_TLD;
  std::string name = STACKOVERFLOW_MEDIA_TYPE;

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
void StackOverflow::GetPublisherPanelInfo(
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
    std::bind(&StackOverflow::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void StackOverflow::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    OnMediaActivityError(window_id);
  } else {
    ledger_->OnPanelPublisherInfo(result, std::move(info), window_id);
  }
}

void StackOverflow::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   "",
                   "",
                   ledger::URL_METHOD::GET,
                   callback);
}

void StackOverflow::OnSaveMediaVisit(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
}

void StackOverflow::SavePublisherInfo(
    const std::string& media_key,
    const std::string& user_id,
    const std::string& publisher_name,
    const std::string& profile_url,
    const std::string& profile_image,
    const uint64_t window_id,
    ledger::PublisherInfoCallback callback) {
  const std::string publisher_key = GetPublisherKey(user_id);

  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  ledger::VisitData visit_data;
  visit_data.provider = STACKOVERFLOW_MEDIA_TYPE;
  visit_data.url = profile_url;
  visit_data.favicon_url = profile_image;
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

} // namespace braveledger_media