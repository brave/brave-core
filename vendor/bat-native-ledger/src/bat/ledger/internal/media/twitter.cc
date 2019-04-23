/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <memory>
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
  return base::StringPrintf("https://twitter.com/%s/", screen_name.c_str());
}

// static
std::string MediaTwitter::GetProfileImageURL(const std::string& screen_name) {
  return base::StringPrintf(
      "https://twitter.com/%s/profile_image?size=original",
      screen_name.c_str());
}

// static
std::string MediaTwitter::GetPublisherKey(const std::string& key) {
  return (std::string)TWITTER_MEDIA_TYPE + "#channel:" + key;
}

void MediaTwitter::SaveMediaInfo(const std::map<std::string, std::string>& data,
                                 ledger::SaveMediaInfoCallback callback) {
  auto user_id = data.find("user_id");
  auto screen_name = data.find("screen_name");
  if (user_id == data.end() || screen_name == data.end()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  const std::string publisher_key = GetPublisherKey(user_id->second);
  const std::string url = GetProfileURL(screen_name->second);
  const std::string favicon_url = GetProfileImageURL(screen_name->second);

  auto name = data.find("name");
  std::string publisher_name = screen_name->second;
  if (name != data.end()) {
    publisher_name = name->second;
  }

  ledger::VisitData visit_data;
  visit_data.favicon_url = favicon_url;
  visit_data.provider = TWITTER_MEDIA_TYPE;
  visit_data.name = publisher_name;
  visit_data.url = url;

  // TODO(nejczdovc): SaveMediaVisit needs to have callback
  ledger_->SaveMediaVisit(publisher_key,
                          visit_data,
                          0,
                          0);

  // Only temp until we create callback
  auto publisher_info = std::make_unique<ledger::PublisherInfo>();
  publisher_info->id = publisher_key;
  publisher_info->name = publisher_name;
  publisher_info->favicon_url = favicon_url;
  publisher_info->provider = TWITTER_MEDIA_TYPE;
  callback(ledger::Result::LEDGER_OK, std::move(publisher_info));
}

}  // namespace braveledger_media
