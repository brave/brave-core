
/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/helper.h"
#include "bat/ledger/internal/media/medium.h"
#include "net/http/http_status_code.h"
#include "url/url_canon.h"
#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

Medium::Medium(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

Medium::~Medium() {
}

void Medium::ProcessActivityFromUrl(
    uint64_t window_id,
    const ledger::VisitData& visit_data) {
  if (visit_data.path.find("/@") != std::string::npos) {
    UserPath(window_id, visit_data);
    return;
  }
  OnMediaActivityError(visit_data, window_id);
}

void Medium::OnMediaActivityError(
    const ledger::VisitData& visit_data,
    uint64_t window_id) {

  ledger::VisitData new_visit_data;
  new_visit_data.domain = MEDIUM_TLD;
  new_visit_data.url = "https://" + (std::string)MEDIUM_TLD;
  new_visit_data.path = "/";
  new_visit_data.name = MEDIUM_MEDIA_TYPE;

  ledger_->GetPublisherActivityFromUrl(
      window_id, ledger::VisitData::New(new_visit_data), std::string());
}

void Medium::UserPath(
    uint64_t window_id,
    const ledger::VisitData& visit_data) {
  const std::string user = GetUserNameFromUrl(visit_data.path);

  if (user.empty()) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  const std::string media_key = (std::string)MEDIUM_MEDIA_TYPE + "_" + user;
  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&Medium::OnUserActivity,
          this,
          window_id,
          visit_data,
          _1,
          _2));
}

void Medium::OnUserActivity(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromUrl(visit_data.path);
    // PRofile URL -> Gets used by FetchUserData?
    const std::string url = GetProfileUrl(user_name);
    FetchDataFromUrl(visit_data.url,
        std::bind(&Medium::OnUserPage,
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

void Medium::OnPageDataFetched(
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

  const std::string user_id = GetUserId(response, user_name);

  SavePublisherInfo(
      0,
      user_name,
      user_id,
      publisher_name,
      callback,
      response);
}

void Medium::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  GURL medium_url(url);

  ledger_->LoadURL(medium_url.spec(),
      std::vector<std::string>(),
      std::string(),
      std::string(),
      ledger::URL_METHOD::GET,
      callback);
}

// static
std::string Medium::GetUserNameFromUrl(const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  const std::vector<std::string> parts = base::SplitString(
    path, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.size() > 1) {
    // This for cases where we are on a user's page, viewing
    // their medium articles
    const std::vector<std::string> sub_parts = base::SplitString(
      parts.at(1), "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (sub_parts.size() > 1) {
      return sub_parts.at(0);
    } else {
      return parts.at(1);
    }
  }

  return std::string();
}

// static
std::string Medium::GetProfileUrl(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }
  const std::string url_part = "https://" + (std::string)MEDIUM_TLD +
      "/@%s/";
  return base::StringPrintf(url_part.c_str(), screen_name.c_str());
}

// static 
std::string Medium::GetUserId(const std::string& response,
                                       const std::string& user_name) {
  if (response.empty() || user_name.empty()) {
    return std::string();
  }

  const std::string start_string = "\"username\":\"" + user_name + "\",";
  std::string publisher_info = braveledger_media::ExtractData(
    response, start_string, "}");

    LOG(INFO) << "@@@@@@@@@@@@@@@@"+publisher_info;

  std::string user_id = braveledger_media::ExtractData(
    publisher_info, "\"id\":\"$User:", ".userMeta");
    LOG(INFO) << "@@@@@@@@@@@@@@@@"+user_id;


  if (user_id.empty()) {
    return std::string();
  }

  LOG(INFO) << "Dsahdgsaibdsajncasdiuncsdiuncdisuncdisunc"+user_id;

  return user_id;

}

// static
std::string Medium::GetPublisherName(const std::string& response,
                                       const std::string& user_name) {
  if (response.empty() || user_name.empty()) {
    return std::string();
  }

  const std::string start_string = "\"username\":\"" + user_name + "\",";
  // Scrapes object 
  std::string publisher_info = braveledger_media::ExtractData(
    response, start_string, "}");

  std::string publisher_name = braveledger_media::ExtractData(
    publisher_info, "\"name\":\"", "\",");

  if (publisher_name.empty()) {
    return std::string();
  }

  return publisher_name;
}

// static
std::string Medium::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }

  return (std::string)MEDIUM_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string Medium::GetProfileImageUrl(const std::string& response) {
  return std::string();
}

void Medium::GetPublisherPanelInfo(
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
    std::bind(&Medium::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void Medium::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(visit_data.url,
                     std::bind(&Medium::OnUserPage,
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

void Medium::OnUserPage(
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
  const std::string user_id = GetUserId(response, user_name);
  const std::string publisher_name = GetPublisherName(response,
                                                      user_name);

  SavePublisherInfo(
      window_id,
      user_name,
      user_id,
      publisher_name,
      std::bind(&Medium::OnMediumSaved,
          this,
          _1,
          _2),
          response);
}

void Medium::OnMediumSaved(
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
}

void Medium::OnMediaPublisherInfo(
    const std::string& user_name,
    ledger::PublisherInfoCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }
  GURL url(MEDIUM_USER_URL + ledger_->URIEncode(user_name));
  if (!url.is_valid()) {
    callback(ledger::Result::TIP_ERROR, nullptr);
    return;
  }

  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(url.spec(),
        std::bind(&Medium::OnPageDataFetched,
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

void Medium::SavePublisherInfo(
    uint64_t window_id,
    const std::string& user_name,
    const std::string& user_id,
    const std::string& publisher_name,
    ledger::PublisherInfoCallback callback,
    const std::string& data) {
    const std::string publisher_key = GetPublisherKey(user_id);
    const std::string media_key = braveledger_media::GetMediaKey(
      user_name,
      MEDIUM_MEDIA_TYPE);


  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  const std::string url = GetProfileUrl(user_name);
  const std::string favicon_url = GetProfileImageUrl(data);

  ledger::VisitDataPtr visit_data = ledger::VisitData::New();
  visit_data->provider = MEDIUM_MEDIA_TYPE;
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

void Medium::SaveMediaInfo(
    const std::map<std::string, std::string>& data,
    ledger::PublisherInfoCallback callback) {
  auto user_name = data.find("user_name");
  if (user_name == data.end()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  const std::string media_key =
      braveledger_media::GetMediaKey(user_name->second, MEDIUM_MEDIA_TYPE);

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&Medium::OnMediaPublisherInfo,
                this,
                user_name->second,
                callback,
                _1,
                _2));
}

}  // namespace braveledger_media 