/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_REDDIT_H_
#define BRAVELEDGER_MEDIA_REDDIT_H_

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

class Reddit {
 public:
  explicit Reddit(bat_ledger::LedgerImpl* ledger);

  ~Reddit();

  void ProcessActivityFromUrl(
      uint64_t window_id,
      const ledger::VisitData& visit_data);

  void SaveMediaInfo(
      const std::map<std::string, std::string>& data,
      ledger::PublisherInfoCallback callback);

 private:
  void OnMediaActivityError(
      const ledger::VisitData& visit_data,
      uint64_t window_id);

  void UserPath(
      uint64_t window_id,
      const ledger::VisitData& visit_data);

  void OnUserActivity(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

  void FetchDataFromUrl(
      const std::string& url,
      braveledger_media::FetchDataFromUrlCallback callback);

  void GetPublisherPanelInfo(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const std::string& publisher_key);

  void OnUserPage(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnPublisherPanelInfo(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const std::string& publisher_key,
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void OnMediaPublisherInfo(
      const std::string& user_name,
      ledger::PublisherInfoCallback callback,
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

  void SavePublisherInfo(
      uint64_t window_id,
      const std::string& user_name,
      ledger::PublisherInfoCallback callback,
      const std::string& data);

  void OnRedditSaved(
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

  static std::string GetUserNameFromUrl(const std::string& path);

  static std::string GetProfileUrl(const std::string& screen_name);

  static std::string GetUserId(const std::string& response);

  static std::string GetPublisherName(const std::string& response);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetProfileImageUrl(const std::string& response);

  void OnPageDataFetched(
      const std::string& user_name,
      ledger::PublisherInfoCallback callback,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  // For testing purposes
  friend class MediaRedditTest;
  FRIEND_TEST_ALL_PREFIXES(MediaRedditTest, GetProfileUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaRedditTest, GetProfileImageUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaRedditTest, GetPublisherKey);
  FRIEND_TEST_ALL_PREFIXES(MediaRedditTest, GetMediaKey);
  FRIEND_TEST_ALL_PREFIXES(MediaRedditTest, GetUserNameFromUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaRedditTest, GetUserId);
  FRIEND_TEST_ALL_PREFIXES(MediaRedditTest, GetPublisherName);
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_REDDIT_H_
