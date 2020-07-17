/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_TWITTER_H_
#define BRAVELEDGER_MEDIA_TWITTER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/media/helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_media {

class Twitter {
 public:
  explicit Twitter(bat_ledger::LedgerImpl* ledger);

  ~Twitter();

  void SaveMediaInfo(const std::map<std::string, std::string>& data,
                     ledger::PublisherInfoCallback callback);

  static std::string GetShareURL(
      const std::map<std::string, std::string>& args);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::VisitData& visit_data);

 private:
  static std::string GetProfileURL(const std::string& screen_name,
                                   const std::string& user_id);

  static std::string GetProfileImageURL(const std::string& screen_name);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetMediaKey(const std::string& screen_name);

  static std::string GetUserNameFromUrl(const std::string& url);

  static bool IsExcludedPath(const std::string& path);

  static std::string GetUserId(const std::string& response);

  static std::string GetPublisherName(const std::string& response);

  void OnMediaPublisherInfo(
      uint64_t window_id,
      const std::string& user_id,
      const std::string& screen_name,
      const std::string& publisher_name,
      ledger::PublisherInfoCallback callback,
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

  void SavePublisherInfo(
      const uint64_t duration,
      const std::string& user_id,
      const std::string& screen_name,
      const std::string& publisher_name,
      const uint64_t window_id,
      ledger::PublisherInfoCallback callback);

  void OnMediaActivityError(const ledger::VisitData& visit_data,
                            uint64_t window_id);

  void FetchDataFromUrl(
      const std::string& url,
      ledger::LoadURLCallback callback);

  void OnMediaPublisherActivity(
      ledger::Result result,
      ledger::PublisherInfoPtr info,
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const std::string& media_key);

  void GetPublisherPanelInfo(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const std::string& publisher_key);

  void OnPublisherPanelInfo(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const std::string& publisher_key,
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void OnUserPage(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const ledger::UrlResponse& response);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  // For testing purposes
  friend class MediaTwitterTest;
  FRIEND_TEST_ALL_PREFIXES(MediaTwitterTest, GetProfileURL);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitterTest, GetProfileImageURL);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitterTest, GetPublisherKey);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitterTest, GetMediaKey);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitterTest, GetUserNameFromUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitterTest, IsExcludedPath);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitterTest, GetUserId);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitterTest, GetPublisherName);
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_TWITTER_H_
