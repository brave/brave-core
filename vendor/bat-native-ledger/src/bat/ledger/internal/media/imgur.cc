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
#include "bat/ledger/internal/media/imgur.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

Imgur::Imgur(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

Imgur::~Imgur() {
}

// static
bool Imgur::GetJSONIntValue(const std::string& key,
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
bool Imgur::GetJSONStringValue(const std::string& key,
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
std::string Imgur::GetProfileUrl(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }
  const std::string url_part = "https://" + (std::string)IMGUR_TLD +
      "/user/%s/";
  return base::StringPrintf(url_part.c_str(), screen_name.c_str());
}

// static
std::string Imgur::GetUserNameFromUrl(const std::string& path) {
  if (path.empty()) {
    return std::string();
  }
  std::vector<std::string> parts = base::SplitString(
      path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.size() > 1) {
    return parts.at(1);
  }

  return std::string();
}

// static
std::string Imgur::GetUserName(const std::string& json_string) {
  std::string publisher_name;
  bool success = GetJSONStringValue("login", json_string, &publisher_name);
  return success ? publisher_name : "";
}

// static
std::string Imgur::GetMediaKey(const std::string& screen_name) {
  if (screen_name.empty()) {
    return "";
  }

  return (std::string)IMGUR_MEDIA_TYPE + "_" + screen_name;
}

// static
std::string Imgur::GetUserId(const std::string& json_string) {
  int64_t user_id;
  const bool success = GetJSONIntValue("id", json_string, &user_id);
  return success ? std::to_string(user_id) : "";
}

// static
std::string Imgur::GetPublisherName(const std::string& response,
                                    const std::string& user_name) {
  if (response.empty() || user_name.empty()) {
    return std::string();
  }

  const std::string start_string = "\"" + user_name + "\",\"name\":\"";

  std::string publisher_name = braveledger_media::ExtractData(
    response, start_string, "\"");

  if (publisher_name.empty()) {
    return std::string();
  }

  return publisher_name;
}

// static
std::string Imgur::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }

  return (std::string)IMGUR_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string Imgur::GetProfileImageUrl(const std::string& json_string) {
  std::string image_url;
	return std::string();
}

void Imgur::ProcessActivityFromUrl(uint64_t window_id,
    const ledger::VisitData& visit_data) {

  if (visit_data.path.find("/user/") != std::string::npos) {
    UserPath(window_id, visit_data);
    return;
  }

  OnMediaActivityError(visit_data, window_id);
  //const std::string user_name = GetUserNameFromUrl(visit_data.path);
  //const std::string media_key = GetMediaKey(user_name);

  //if (media_key.empty()) {
  //  OnMediaActivityError(window_id);
  //  return;
  //}

  //ledger_->GetMediaPublisherInfo(
  //    media_key,
  //    std::bind(&Imgur::OnUserActivity,
  //              this,
  //              _1,
  //              _2,
  //              window_id,
  //              visit_data,
  //              media_key));
}

void Imgur::UserPath(
    uint64_t window_id,
    const ledger::VisitData& visit_data) {

  const std::string user = Imgur::GetUserNameFromUrl(visit_data.path);
  const std::string media_key = (std::string)IMGUR_MEDIA_TYPE + "_" + user;
  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&Imgur::OnUserActivity,
        this,
        window_id,
        visit_data,
        _1,
        _2));

}

void Imgur::OnUserActivity(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromUrl(visit_data.path);
    const std::string url = GetProfileUrl(user_name);
    FetchDataFromUrl(visit_data.url,
        std::bind(&Imgur::OnUserPage,
            this,
            window_id,
            visit_data,
            _1,
            _2,
            _3));
  } else {
    GetPublisherPanelInfo(
        window_id,
        visit_data,
        publisher_info->id);
  }
}

void Imgur::OnMediaActivityError(
    const ledger::VisitData& visit_data,
    uint64_t window_id) {

  ledger::VisitData new_visit_data;
  new_visit_data.domain = IMGUR_TLD;
  new_visit_data.url = "https://" + (std::string)IMGUR_TLD;
  new_visit_data.path = "/";
  new_visit_data.name = IMGUR_MEDIA_TYPE;

  ledger_->GetPublisherActivityFromUrl(
      window_id, ledger::VisitData::New(new_visit_data), std::string());
}

void Imgur::OnPageDataFetched(
    const std::string& user_name,
    ledger::PublisherInfoCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::TIP_ERROR, nullptr);
    return;
  }

  const std::string publisher_name = GetPublisherName(response,
                                                      user_name);

  SavePublisherInfo(
      0,
      user_name,
      publisher_name,
      callback,
      response);
}

void Imgur::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  GURL imgur_url(url);

  ledger_->LoadURL(imgur_url.spec(),
                   std::vector<std::string>(),
                   std::string(),
                   std::string(),
                   ledger::URL_METHOD::GET,
                   callback);
}

void Imgur::GetPublisherPanelInfo(
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
    std::bind(&Imgur::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void Imgur::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(visit_data.url,
                     std::bind(&Imgur::OnUserPage,
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

void Imgur::OnUserPage(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  const std::string user_name = GetUserNameFromUrl(visit_data.path);
  //const std::string publisher_name = GetPublisherName(response,
  //                                                    user_name);
  const std::string publisher_name = user_name;

  SavePublisherInfo(
      window_id,
      user_name,
      publisher_name,
      std::bind(&Imgur::OnImgurSaved,
          this,
          _1,
          _2),
          response);
}

void Imgur::OnImgurSaved(
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
}

void Imgur::OnSaveMediaVisit(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
}

void Imgur::SavePublisherInfo(
    uint64_t window_id,
    const std::string& user_name,
    const std::string& publisher_name,
    ledger::PublisherInfoCallback callback,
    const std::string& data) {
  const std::string publisher_key = GetPublisherKey(user_name);
  const std::string media_key = braveledger_media::GetMediaKey(
      user_name,
      IMGUR_MEDIA_TYPE);

  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  const std::string url = GetProfileUrl(user_name);
  const std::string favicon_url = GetProfileImageUrl(data);

  ledger::VisitDataPtr visit_data = ledger::VisitData::New();
  visit_data->provider = IMGUR_MEDIA_TYPE;
  visit_data->url = url;
  visit_data->favicon_url = favicon_url;
  visit_data->name = publisher_name;


  ledger_->SaveMediaVisit(publisher_key,
                          *visit_data,
                          0,
                          window_id,
                          callback);

  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, publisher_key);
  }
}

void Imgur::OnMediaPublisherInfo(
    const std::string& user_name,
    ledger::PublisherInfoCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }
  GURL url(IMGUR_USER_URL + ledger_->URIEncode(user_name));
  if (!url.is_valid()) {
    callback(ledger::Result::TIP_ERROR, nullptr);
    return;
  }

  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(url.spec(),
        std::bind(&Imgur::OnPageDataFetched,
          this,
          user_name,
          callback,
          _1,
          _2,
          _3));
  } else {
    callback(result, std::move(publisher_info));
  }
}

void Imgur::SaveMediaInfo(
    const std::map<std::string, std::string>& data,
    ledger::PublisherInfoCallback callback) {
  auto user_name = data.find("user_name");
  if (user_name == data.end()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  const std::string media_key =
      braveledger_media::GetMediaKey(user_name->second, IMGUR_MEDIA_TYPE);

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&Imgur::OnMediaPublisherInfo,
                this,
                user_name->second,
                callback,
                _1,
                _2));
}



}  // namespace braveledger_media
