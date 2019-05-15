/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/helper.h"
#include "bat/ledger/internal/media/twitter.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_media {

MediaTwitter::MediaTwitter(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

MediaTwitter::~MediaTwitter() {
}

// static
std::string MediaTwitter::GetProfileURL(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }

  return base::StringPrintf("https://twitter.com/%s/", screen_name.c_str());
}

// static
std::string MediaTwitter::GetProfileImageURL(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }

  return base::StringPrintf(
      "https://twitter.com/%s/profile_image?size=original",
      screen_name.c_str());
}

// static
std::string MediaTwitter::GetPublisherKey(const std::string& key) {
  if (key.empty()) {
    return std::string();
  }

  return (std::string)TWITTER_MEDIA_TYPE + "#channel:" + key;
}

// static
std::string MediaTwitter::GetMediaKey(const std::string& screen_name) {
  if (screen_name.empty()) {
    return std::string();
  }

  return (std::string)TWITTER_MEDIA_TYPE + "_" + screen_name;
}

void MediaTwitter::SaveMediaInfo(const std::map<std::string, std::string>& data,
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
          std::bind(&MediaTwitter::OnMediaPublisherInfo,
                    this,
                    0,
                    user_id->second,
                    screen_name->second,
                    publisher_name,
                    callback,
                    _1,
                    _2));
}

void MediaTwitter::OnMediaPublisherInfo(
    uint64_t window_id,
    const std::string& user_id,
    const std::string& screen_name,
    const std::string& publisher_name,
    ledger::PublisherInfoCallback callback,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
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

void MediaTwitter::SavePublisherInfo(
    const uint64_t duration,
    const std::string& user_id,
    const std::string& screen_name,
    const std::string& publisher_name,
    const uint64_t window_id,
    ledger::PublisherInfoCallback callback) {
  const std::string publisher_key = GetPublisherKey(user_id);
  const std::string url = GetProfileURL(screen_name);
  const std::string favicon_url = GetProfileImageURL(screen_name);
  const std::string media_key = GetMediaKey(screen_name);

  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher key is missing for: " << media_key;
    return;
  }

  ledger::VisitData visit_data;
  visit_data.provider = TWITTER_MEDIA_TYPE;
  visit_data.url = url;
  visit_data.favicon_url = favicon_url;
  visit_data.name = publisher_name;

  ledger_->SaveMediaVisit(publisher_key,
                          visit_data,
                          duration,
                          window_id,
                          callback);
  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, publisher_key);
  }
}

}  // namespace braveledger_media
