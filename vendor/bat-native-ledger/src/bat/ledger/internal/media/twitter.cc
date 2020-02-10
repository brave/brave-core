/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/helper.h"
#include "bat/ledger/internal/media/twitter.h"
#include "bat/ledger/internal/static_values.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

std::string GetUserIdFromUrl(const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  GURL url("https://twitter.com" + path);
  if (!url.is_valid()) {
    return std::string();
  }

  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    if (it.GetKey() == "user_id") {
      return it.GetUnescapedValue();
    }
  }

  return std::string();
}

}  // namespace

namespace braveledger_media {

Twitter::Twitter(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

Twitter::~Twitter() {
}

// static
std::string Twitter::GetProfileURL(const std::string& screen_name,
                                   const std::string& user_id) {
  if (!user_id.empty()) {
    return base::StringPrintf("https://twitter.com/intent/user?user_id=%s",
                              user_id.c_str());
  }

  if (!screen_name.empty()) {
    return base::StringPrintf("https://twitter.com/%s/", screen_name.c_str());
  }

  return "";
}

// static
std::string Twitter::GetProfileImageURL(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }

  return base::StringPrintf(
      "https://twitter.com/%s/profile_image?size=original",
      screen_name.c_str());
}

// static
std::string Twitter::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }

  return (std::string)TWITTER_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string Twitter::GetMediaKey(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }

  return (std::string)TWITTER_MEDIA_TYPE + "_" + screen_name;
}

// static
std::string Twitter::GetUserNameFromUrl(const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  GURL url("https://twitter.com" + path);
  if (!url.is_valid()) {
    return std::string();
  }

  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    if (it.GetKey() == "screen_name") {
      return it.GetUnescapedValue();
    }
  }

  std::vector<std::string> parts = base::SplitString(
      path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.size() > 0) {
    return parts.at(0);
  }

  return std::string();
}

// static
bool Twitter::IsExcludedPath(const std::string& path) {
  if (path.empty()) {
    return true;
  }

  const std::vector<std::string> paths({
      "/",
      "/settings",
      "/explore",
      "/notifications",
      "/messages",
      "/logout",
      "/search",
      "/about",
      "/tos",
      "/privacy",
      "/home"
    });

  for (std::string str_path : paths) {
    if (str_path == path || str_path + "/" == path) {
      return true;
    }
  }

  const std::vector<std::string> patterns({
    "/i/",
    "/account/",
    "/compose/",
    "/?login",
    "/?logout",
    "/who_to_follow/",
    "/hashtag/",
    "/settings/"
  });

  for (std::string str_path : patterns) {
    if (base::StartsWith(path,
                         str_path,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }


  return false;
}

// static
std::string Twitter::GetUserId(const std::string& response) {
  if (response.empty()) {
    return std::string();
  }

  std::string id = braveledger_media::ExtractData(
      response,
      "<a href=\"/intent/user?user_id=\"", "\">");

  if (id.empty()) {
    id = braveledger_media::ExtractData(
        response,
        "<div class=\"ProfileNav\" role=\"navigation\" data-user-id=\"", "\">");
  }

  if (id.empty()) {
    id = braveledger_media::ExtractData(
        response, "https://pbs.twimg.com/profile_banners/", "/");
  }

  return id;
}

// static
std::string Twitter::GetPublisherName(const std::string& response) {
  if (response.empty()) {
    return std::string();
  }

  const std::string title = braveledger_media::ExtractData(
      response, "<title>", "</title>");

  if (title.empty()) {
    return std::string();
  }

  std::vector<std::string> parts = base::SplitStringUsingSubstr(
      title, " (@", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (parts.size() > 0) {
    return parts.at(0);
  }

  return title;
}

void Twitter::SaveMediaInfo(const std::map<std::string, std::string>& data,
                                 ledger::PublisherInfoCallback callback) {
  auto user_id = data.find("user_id");
  auto screen_name = data.find("screen_name");
  if (user_id == data.end() || screen_name == data.end()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  const std::string media_key = GetMediaKey(screen_name->second);

  auto name = data.find("name");
  std::string publisher_name = screen_name->second;
  if (name != data.end()) {
    publisher_name = name->second;
  }

  ledger_->GetMediaPublisherInfo(
          media_key,
          std::bind(&Twitter::OnMediaPublisherInfo,
                    this,
                    0,
                    user_id->second,
                    screen_name->second,
                    publisher_name,
                    callback,
                    _1,
                    _2));
}

// static
std::string Twitter::GetShareURL(
    const std::map<std::string, std::string>& args) {
  auto comment = args.find("comment");
  auto name = args.find("name");
  auto tweet_id = args.find("tweet_id");
  auto hashtag = args.find("hashtag");
  if (comment == args.end() || name == args.end() || hashtag == args.end())
    return std::string();

  // Append hashtag to comment ("%20%23" = percent-escaped space and
  // number sign)
  std::string comment_with_hashtag =
      comment->second + "%20%23" + hashtag->second;

  // If a tweet ID was specified, then quote the original tweet along
  // with the supplied comment; otherwise, just tweet the comment.
  std::string share_url;
  if (tweet_id != args.end() && !tweet_id->second.empty()) {
    std::string quoted_tweet_url =
        base::StringPrintf("https://twitter.com/%s/status/%s",
                           name->second.c_str(), tweet_id->second.c_str());
    share_url = base::StringPrintf(
        "https://twitter.com/intent/tweet?text=%s&url=%s",
        comment_with_hashtag.c_str(), quoted_tweet_url.c_str());
  } else {
    share_url = base::StringPrintf("https://twitter.com/intent/tweet?text=%s",
                                   comment_with_hashtag.c_str());
  }
  return share_url;
}

void Twitter::OnMediaPublisherInfo(
    uint64_t window_id,
    const std::string& user_id,
    const std::string& screen_name,
    const std::string& publisher_name,
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
                      window_id,
                      callback);
  } else {
    // TODO(nejczdovc): we need to check if user is verified,
    //  but his image was not saved yet, so that we can fix it
    callback(result, std::move(publisher_info));
  }
}

void Twitter::SavePublisherInfo(
    const uint64_t duration,
    const std::string& user_id,
    const std::string& screen_name,
    const std::string& publisher_name,
    const uint64_t window_id,
    ledger::PublisherInfoCallback callback) {
  const std::string publisher_key = GetPublisherKey(user_id);
  const std::string url = GetProfileURL(screen_name, user_id);
  const std::string favicon_url = GetProfileImageURL(screen_name);
  const std::string media_key = GetMediaKey(screen_name);

  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  ledger::VisitDataPtr visit_data = ledger::VisitData::New();
  visit_data->provider = TWITTER_MEDIA_TYPE;
  visit_data->url = url;
  visit_data->favicon_url = favicon_url;
  visit_data->name = publisher_name;

  ledger_->SaveMediaVisit(publisher_key,
                          *visit_data,
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

void Twitter::OnSaveMediaVisit(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
}

void Twitter::FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   std::string(),
                   std::string(),
                   ledger::UrlMethod::GET,
                   callback);
}

void Twitter::OnMediaActivityError(const ledger::VisitData& visit_data,
                                        uint64_t window_id) {
  std::string url = TWITTER_TLD;
  std::string name = TWITTER_MEDIA_TYPE;

  DCHECK(!url.empty());

  ledger::VisitData new_visit_data;
  new_visit_data.domain = url;
  new_visit_data.url = "https://" + url;
  new_visit_data.path = "/";
  new_visit_data.name = name;

  ledger_->GetPublisherActivityFromUrl(
      window_id, ledger::VisitData::New(new_visit_data), std::string());
}

void Twitter::ProcessActivityFromUrl(
    uint64_t window_id,
    const ledger::VisitData& visit_data) {
  // not all url's are publisher specific
  if (IsExcludedPath(visit_data.path)) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  const std::string user_name = GetUserNameFromUrl(visit_data.path);
  const std::string media_key = GetMediaKey(user_name);

  if (media_key.empty()) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  ledger_->GetMediaPublisherInfo(
      media_key,
      std::bind(&Twitter::OnMediaPublisherActivity,
                this,
                _1,
                _2,
                window_id,
                visit_data,
                media_key));
}

void Twitter::OnMediaPublisherActivity(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& media_key) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  if (!info || result == ledger::Result::NOT_FOUND) {
    const std::string user_name = GetUserNameFromUrl(visit_data.path);
    const std::string user_id = GetUserIdFromUrl(visit_data.path);
    const std::string url = GetProfileURL(user_name, user_id);

    FetchDataFromUrl(url,
                     std::bind(&Twitter::OnUserPage,
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

// Gets publisher panel info where we know that publisher info exists
void Twitter::GetPublisherPanelInfo(
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
    std::bind(&Twitter::OnPublisherPanelInfo,
              this,
              window_id,
              visit_data,
              publisher_key,
              _1,
              _2));
}

void Twitter::OnPublisherPanelInfo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& publisher_key,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (!info || result == ledger::Result::NOT_FOUND) {
    FetchDataFromUrl(visit_data.url,
                     std::bind(&Twitter::OnUserPage,
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

void Twitter::OnUserPage(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != net::HTTP_OK) {
    OnMediaActivityError(visit_data, window_id);
    return;
  }

  const std::string user_id = GetUserId(response);
  const std::string user_name = GetUserNameFromUrl(visit_data.path);
  std::string publisher_name = GetPublisherName(response);

  if (publisher_name.empty()) {
    publisher_name = user_name;
  }

  auto callback = std::bind(&Twitter::OnSaveMediaVisit,
                            this,
                            _1,
                            _2);

  SavePublisherInfo(0,
                    user_id,
                    user_name,
                    publisher_name,
                    window_id,
                    callback);
}

}  // namespace braveledger_media
