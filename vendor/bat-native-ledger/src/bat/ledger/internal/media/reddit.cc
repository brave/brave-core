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
#include "bat/ledger/internal/media/reddit.h"
#include "net/http/http_status_code.h"
#include "url/url_canon.h"
#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

MediaReddit::MediaReddit(bat_ledger::LedgerImpl* ledger): ledger_(ledger) {
}

MediaReddit::~MediaReddit() {
}

void MediaReddit::ProcessActivityFromUrl(
    uint64_t window_id,
    const ledger::VisitData& visit_data) {
  if (visit_data.path.find("/user/") != std::string::npos) {
    UserPath(window_id, visit_data);
    return;
  }
  OnMediaActivityError(visit_data, window_id);
}

void MediaReddit::OnMediaActivityError(
    const ledger::VisitData& visit_data,
    uint64_t window_id) {

  ledger::VisitData new_visit_data;
  new_visit_data.domain = REDDIT_TLD;
  new_visit_data.url = "https://" + (std::string)REDDIT_TLD;
  new_visit_data.path = "/";
  new_visit_data.name = REDDIT_MEDIA_TYPE;

  ledger_->GetPublisherActivityFromUrl(
      window_id, new_visit_data, std::string());
}

void MediaReddit::UserPath(
    uint64_t window_id,
    const ledger::VisitData& visit_data) {
  const std::string user = GetUserNameFromUrl(visit_data.path);

  if (user.empty()) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  const std::string media_key = (std::string)REDDIT_MEDIA_TYPE + "_" + user;
  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&MediaReddit::OnUserActivity,
          this,
          window_id,
          visit_data,
          media_key,
          _1,
          _2));
}

void MediaReddit::OnUserActivity(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& media_key,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromUrl(visit_data.path);
    const std::string url = GetProfileUrl(user_name);
    FetchDataFromUrl(visit_data.url,
        std::bind(&MediaReddit::OnUserPage,
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

void MediaReddit::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  /* if user is on old reddit, sub the url to get the icon
     since old reddit didn't have user icons */
  GURL reddit_url(url);
  if (reddit_url.DomainIs(OLD_REDDIT_DOMAIN)) {
    // Canonicalize away 'old.reddit.com' and replace with 'www.reddit.com'.
    std::string new_host = reddit_url.host();
    new_host.replace(0, 3, "www");
    url::Replacements<char> replacements;
    replacements.SetHost(new_host.c_str(),
                         url::Component(0, new_host.length()));
    reddit_url = reddit_url.ReplaceComponents(replacements);
  }

  ledger_->LoadURL(reddit_url.spec(),
      std::vector<std::string>(),
      std::string(),
      std::string(),
      ledger::URL_METHOD::GET,
      callback);
}

// static
std::string MediaReddit::GetUserNameFromUrl(const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  const std::vector<std::string> parts = base::SplitString(
    path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.size() > 1) {
    return parts.at(1);
  }

  return std::string();
}

// static
std::string MediaReddit::GetProfileUrl(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }
  const std::string url_part = "https://" + (std::string)REDDIT_TLD +
      "/user/%s/";
  return base::StringPrintf(url_part.c_str(), screen_name.c_str());
}

void MediaReddit::GetPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key) {
  const auto filter = ledger_->CreateActivityFilter(
    publisher_key,
    ledger::EXCLUDE_FILTER::FILTER_ALL,
    false,
    ledger_->GetReconcileStamp(),
    true,
    false);
  ledger_->GetPanelPublisherInfo(filter,
    std::bind(&MediaReddit::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void MediaReddit::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(visit_data.url,
                     std::bind(&MediaReddit::OnUserPage,
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

// static
std::string MediaReddit::GetUserId(const std::string& response) {
  if (response.empty()) {
    return std::string();
  }
  const std::string pattern = braveledger_media::ExtractData(
      response, "hideFromRobots\":", "\"isEmployee\"");
  std::string id = braveledger_media::ExtractData(
      pattern, "\"id\":\"t2_", "\"");

  if (id.empty()) {
    id = braveledger_media::ExtractData(
        response, "target_fullname\": \"t2_", "\"");  // old reddit
  }
  return id;
}

// static
std::string MediaReddit::GetPublisherName(const std::string& response) {
  if (response.empty()) {
    return std::string();
  }

  std::string user_name(braveledger_media::ExtractData(
      response, "username\":\"", "\""));

  if (user_name.empty()) {
    user_name = braveledger_media::ExtractData(
        response, "target_name\": \"", "\"");  // old reddit
  }
  return user_name;
}

void MediaReddit::OnUserPage(
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
  const std::string publisher_name = GetPublisherName(response);
  const std::string user_id = GetUserId(response);
  const std::string publisher_key = GetPublisherKey(user_id);
  const std::string media_key = GetMediaKey(user_name, REDDIT_MEDIA_TYPE);

  ledger::VisitData new_visit_data(visit_data);
  new_visit_data.provider = REDDIT_MEDIA_TYPE;
  new_visit_data.url = GetProfileUrl(user_name);
  new_visit_data.favicon_url = GetProfileImageUrl(response);
  new_visit_data.name = publisher_name.empty() ? user_name : publisher_name;

  ledger_->SaveMediaVisit(
    publisher_key,
    new_visit_data,
    0,
    window_id,
    std::bind(&MediaReddit::OnUserActivity,
          this,
          window_id,
          visit_data,
          user_id,
          _1,
          _2));

  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, publisher_key);
  }
}

// static
std::string MediaReddit::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }
  return (std::string)REDDIT_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string MediaReddit::GetProfileImageUrl(const std::string& response) {
  if (response.empty()) {
    return std::string();
  }

  const std::string image_url(braveledger_media::ExtractData(
      response, "accountIcon\":\"", "?"));
  return image_url;  // old reddit does not use account icons
}

}  // namespace braveledger_media
