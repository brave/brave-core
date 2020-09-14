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
#include "bat/ledger/internal/legacy/media/helper.h"
#include "bat/ledger/internal/legacy/media/reddit.h"
#include "bat/ledger/internal/legacy/static_values.h"
#include "bat/ledger/internal/constants.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"
#include "url/url_canon.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

Reddit::Reddit(ledger::LedgerImpl* ledger): ledger_(ledger) {
}

Reddit::~Reddit() {
}

void Reddit::ProcessActivityFromUrl(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data) {
  if (visit_data.path.find("/user/") != std::string::npos) {
    UserPath(window_id, visit_data);
    return;
  }
  OnMediaActivityError(visit_data, window_id);
}

void Reddit::OnMediaActivityError(
    const ledger::type::VisitData& visit_data,
    uint64_t window_id) {

  ledger::type::VisitData new_visit_data;
  new_visit_data.domain = REDDIT_TLD;
  new_visit_data.url = "https://" + (std::string)REDDIT_TLD;
  new_visit_data.path = "/";
  new_visit_data.name = REDDIT_MEDIA_TYPE;

  ledger_->publisher()->GetPublisherActivityFromUrl(
      window_id, ledger::type::VisitData::New(new_visit_data), std::string());
}

void Reddit::UserPath(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data) {
  const std::string user = GetUserNameFromUrl(visit_data.path);

  if (user.empty()) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  const std::string media_key = (std::string)REDDIT_MEDIA_TYPE + "_" + user;
  ledger_->database()->GetMediaPublisherInfo(
      media_key,
      std::bind(&Reddit::OnUserActivity,
          this,
          window_id,
          visit_data,
          _1,
          _2));
}

void Reddit::OnUserActivity(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data,
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr publisher_info) {
  if (!publisher_info || result == ledger::type::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromUrl(visit_data.path);
    const std::string url = GetProfileUrl(user_name);
    auto url_callback = std::bind(&Reddit::OnUserPage,
        this,
        window_id,
        visit_data,
        _1);
    FetchDataFromUrl(visit_data.url, url_callback);
  } else {
    GetPublisherPanelInfo(
        window_id,
        visit_data,
        publisher_info->id);
  }
}

void Reddit::OnPageDataFetched(
    const std::string& user_name,
    ledger::PublisherInfoCallback callback,
    const ledger::type::UrlResponse& response) {
  if (response.status_code != net::HTTP_OK) {
    callback(ledger::type::Result::TIP_ERROR, nullptr);
    return;
  }

  SavePublisherInfo(
      0,
      user_name,
      callback,
      response.body);
}

void Reddit::FetchDataFromUrl(
    const std::string& url,
    ledger::client::LoadURLCallback callback) {
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

  auto request = ledger::type::UrlRequest::New();
  request->url = reddit_url.spec();
  request->skip_log = true;
  ledger_->LoadURL(std::move(request), callback);
}

// static
std::string Reddit::GetUserNameFromUrl(const std::string& path) {
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
std::string Reddit::GetProfileUrl(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }
  const std::string url_part = "https://" + (std::string)REDDIT_TLD +
      "/user/%s/";
  return base::StringPrintf(url_part.c_str(), screen_name.c_str());
}

void Reddit::GetPublisherPanelInfo(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data,
    const std::string& publisher_key) {
  auto filter = ledger_->publisher()->CreateActivityFilter(
    publisher_key,
    ledger::type::ExcludeFilter::FILTER_ALL,
    false,
    ledger_->state()->GetReconcileStamp(),
    true,
    false);
  ledger_->database()->GetPanelPublisherInfo(std::move(filter),
    std::bind(&Reddit::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void Reddit::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr info) {
  if (!info || result == ledger::type::Result::NOT_FOUND) {
    auto url_callback = std::bind(&Reddit::OnUserPage,
        this,
        window_id,
        visit_data,
        _1);

    FetchDataFromUrl(visit_data.url, url_callback);
  } else {
    ledger_->ledger_client()->OnPanelPublisherInfo(
        result,
        std::move(info),
        window_id);
  }
}

// static
std::string Reddit::GetUserId(const std::string& response) {
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
std::string Reddit::GetPublisherName(const std::string& response) {
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

void Reddit::OnRedditSaved(
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr publisher_info) {
}

void Reddit::OnUserPage(
    uint64_t window_id,
    const ledger::type::VisitData& visit_data,
    const ledger::type::UrlResponse& response) {
  if (response.status_code != net::HTTP_OK) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  const std::string user_name = GetUserNameFromUrl(visit_data.path);
  SavePublisherInfo(
      window_id,
      user_name,
      std::bind(&Reddit::OnRedditSaved,
          this,
          _1,
          _2),
          response.body);
}

// static
std::string Reddit::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }
  return (std::string)REDDIT_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string Reddit::GetProfileImageUrl(const std::string& response) {
  if (response.empty()) {
    return std::string();
  }

  const std::string image_url(braveledger_media::ExtractData(
      response, "accountIcon\":\"", "?"));
  return image_url;  // old reddit does not use account icons
}

void Reddit::OnMediaPublisherInfo(
    const std::string& user_name,
    ledger::PublisherInfoCallback callback,
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr publisher_info) {
  if (result != ledger::type::Result::LEDGER_OK &&
      result != ledger::type::Result::NOT_FOUND) {
    callback(ledger::type::Result::LEDGER_ERROR, nullptr);
    return;
  }
  GURL url(REDDIT_USER_URL + ledger_->ledger_client()->URIEncode(user_name));
  if (!url.is_valid()) {
    callback(ledger::type::Result::TIP_ERROR, nullptr);
    return;
  }

  if (!publisher_info || result == ledger::type::Result::NOT_FOUND) {
    FetchDataFromUrl(url.spec(),
        std::bind(&Reddit::OnPageDataFetched,
          this,
          user_name,
          callback,
          _1));
  } else {
    callback(result, std::move(publisher_info));
  }
}

void Reddit::SavePublisherInfo(
    uint64_t window_id,
    const std::string& user_name,
    ledger::PublisherInfoCallback callback,
    const std::string& data) {
  const std::string user_id = GetUserId(data);
  const std::string publisher_key = GetPublisherKey(user_id);
  const std::string media_key = GetMediaKey(user_name, REDDIT_MEDIA_TYPE);
  if (publisher_key.empty()) {
    callback(ledger::type::Result::LEDGER_ERROR, nullptr);
    BLOG(0, "Publisher key is missing for: " << media_key);
    return;
  }

  const std::string url = GetProfileUrl(user_name);
  const std::string favicon_url = GetProfileImageUrl(data);

  ledger::type::VisitDataPtr visit_data = ledger::type::VisitData::New();
  visit_data->provider = REDDIT_MEDIA_TYPE;
  visit_data->url = url;
  visit_data->favicon_url = favicon_url;
  visit_data->name = user_name;

  ledger_->publisher()->SaveVisit(
      publisher_key,
      *visit_data,
      0,
      true,
      window_id,
      callback);

  if (!media_key.empty()) {
    ledger_->database()->SaveMediaPublisherInfo(
        media_key,
        publisher_key,
        [](const ledger::type::Result) {});
  }
}

void Reddit::SaveMediaInfo(
    const std::map<std::string, std::string>& data,
    ledger::PublisherInfoCallback callback) {
  auto user_name = data.find("user_name");
  if (user_name == data.end()) {
    callback(ledger::type::Result::LEDGER_ERROR, nullptr);
    return;
  }

  const std::string media_key =
      braveledger_media::GetMediaKey(user_name->second, REDDIT_MEDIA_TYPE);

  ledger_->database()->GetMediaPublisherInfo(
      media_key,
      std::bind(&Reddit::OnMediaPublisherInfo,
                this,
                user_name->second,
                callback,
                _1,
                _2));
}

}  // namespace braveledger_media
