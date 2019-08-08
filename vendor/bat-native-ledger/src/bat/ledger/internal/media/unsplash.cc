/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <utility>
#include <vector>
#include <iostream>
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/helper.h"
#include "bat/ledger/internal/media/unsplash.h"
#include "net/http/http_status_code.h"
#include "url/url_canon.h"
#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

Unsplash::Unsplash(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

Unsplash::~Unsplash() {
}

void Unsplash::ProcessActivityFromUrl(
    uint64_t window_id,
    const ledger::VisitData& visit_data) {
  if (visit_data.path.find("/@") != std::string::npos) {
    UserPath(window_id, visit_data);
    return;
  }
  OnMediaActivityError(visit_data, window_id);
}

void Unsplash::OnMediaActivityError(
    const ledger::VisitData& visit_data,
    uint64_t window_id) {

  ledger::VisitData new_visit_data;
  new_visit_data.domain = UNSPLASH_TLD;
  new_visit_data.url = "https://" + (std::string)UNSPLASH_TLD;
  new_visit_data.path = "/";
  new_visit_data.name = UNSPLASH_MEDIA_TYPE;

  ledger_->GetPublisherActivityFromUrl(
      window_id, ledger::VisitData::New(new_visit_data), std::string());
}

void Unsplash::UserPath(
    uint64_t window_id,
    const ledger::VisitData& visit_data) {
  const std::string user = GetUserNameFromUrl(visit_data.path);

  if (user.empty()) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  const std::string media_key = (std::string)UNSPLASH_MEDIA_TYPE + "_" + user;
  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&Unsplash::OnUserActivity,
          this,
          window_id,
          visit_data,
          _1,
          _2));
}

void Unsplash::OnUserActivity(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromUrl(visit_data.path);
    const std::string url = GetProfileUrl(user_name);
    FetchDataFromUrl(visit_data.url,
        std::bind(&Unsplash::OnUserPage,
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

void Unsplash::OnPageDataFetched(
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

void Unsplash::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  GURL unsplash_url(url);

  ledger_->LoadURL(unsplash_url.spec(),
      std::vector<std::string>(),
      std::string(),
      std::string(),
      ledger::URL_METHOD::GET,
      callback);
}

// static
std::string Unsplash::GetUserNameFromUrl(const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  const std::vector<std::string> parts = base::SplitString(
    path, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.size() > 1) {
    // This for cases where we are on a user's page, viewing
    // their collections (/collections) or likes (/likes)
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
std::string Unsplash::GetProfileUrl(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }
  const std::string url_part = "https://" + (std::string)UNSPLASH_TLD +
      "/@%s/";
  return base::StringPrintf(url_part.c_str(), screen_name.c_str());
}

// static
std::string Unsplash::GetPublisherName(const std::string& response,
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
std::string Unsplash::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }

  return (std::string)UNSPLASH_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string Unsplash::GetProfileImageUrl(const std::string& response) {
  if (response.empty()) {
    return std::string();
  }

  std::string profile_img_url = braveledger_media::ExtractData(
    response, "\"profile_image\":{\"small\":\"", "\"");

  if (profile_img_url.empty()) {
    return std::string();
  }

  return profile_img_url;
}

void Unsplash::GetPublisherPanelInfo(
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
    std::bind(&Unsplash::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void Unsplash::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(visit_data.url,
                     std::bind(&Unsplash::OnUserPage,
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

void Unsplash::OnUserPage(
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
  const std::string publisher_name = GetPublisherName(response,
                                                      user_name);

  SavePublisherInfo(
      window_id,
      user_name,
      publisher_name,
      std::bind(&Unsplash::OnUnsplashSaved,
          this,
          _1,
          _2),
          response);
}

void Unsplash::OnUnsplashSaved(
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
}

void Unsplash::OnMediaPublisherInfo(
    const std::string& user_name,
    ledger::PublisherInfoCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }
  GURL url(UNSPLASH_USER_URL + ledger_->URIEncode(user_name));
  if (!url.is_valid()) {
    callback(ledger::Result::TIP_ERROR, nullptr);
    return;
  }

  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(url.spec(),
        std::bind(&Unsplash::OnPageDataFetched,
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

void Unsplash::SavePublisherInfo(
    uint64_t window_id,
    const std::string& user_name,
    const std::string& publisher_name,
    ledger::PublisherInfoCallback callback,
    const std::string& data) {
  const std::string publisher_key = GetPublisherKey(user_name);
  const std::string media_key = braveledger_media::GetMediaKey(
      user_name,
      UNSPLASH_MEDIA_TYPE);

  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  const std::string url = GetProfileUrl(user_name);
  const std::string profile_img_url = GetProfileImageUrl(data);
  const std::string display_name = publisher_name.empty()
                                   ? user_name
                                   : publisher_name;

  ledger::VisitDataPtr visit_data = ledger::VisitData::New();
  visit_data->provider = UNSPLASH_MEDIA_TYPE;
  visit_data->url = url;
  visit_data->favicon_url = profile_img_url;
  visit_data->name = display_name;


  ledger_->SaveMediaVisit(publisher_key,
                          *visit_data,
                          0,
                          window_id,
                          callback);

  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, publisher_key);
  }
}

void Unsplash::SaveMediaInfo(
    const std::map<std::string, std::string>& data,
    ledger::PublisherInfoCallback callback) {
  auto user_name = data.find("user_name");
  if (user_name == data.end()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  const std::string media_key =
      braveledger_media::GetMediaKey(user_name->second, UNSPLASH_MEDIA_TYPE);

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&Unsplash::OnMediaPublisherInfo,
                this,
                user_name->second,
                callback,
                _1,
                _2));
}

}  // namespace braveledger_media
