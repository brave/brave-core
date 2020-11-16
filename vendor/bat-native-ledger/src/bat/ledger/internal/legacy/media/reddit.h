/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_REDDIT_H_
#define BRAVELEDGER_MEDIA_REDDIT_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "bat/ledger/internal/legacy/media/helper.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;
}

namespace braveledger_media {

class Reddit {
 public:
  explicit Reddit(ledger::LedgerImpl* ledger);

  ~Reddit();

  void ProcessActivityFromUrl(
      uint64_t window_id,
      const ledger::type::VisitData& visit_data);

  void SaveMediaInfo(
      const base::flat_map<std::string, std::string>& data,
      ledger::PublisherInfoCallback callback);

 private:
  void OnMediaActivityError(
      const ledger::type::VisitData& visit_data,
      uint64_t window_id);

  void UserPath(
      uint64_t window_id,
      const ledger::type::VisitData& visit_data);

  void OnUserActivity(
      uint64_t window_id,
      const ledger::type::VisitData& visit_data,
      ledger::type::Result result,
      ledger::type::PublisherInfoPtr publisher_info);

  void FetchDataFromUrl(
      const std::string& url,
      ledger::client::LoadURLCallback callback);

  void GetPublisherPanelInfo(
      uint64_t window_id,
      const ledger::type::VisitData& visit_data,
      const std::string& publisher_key);

  void OnUserPage(
      uint64_t window_id,
      const ledger::type::VisitData& visit_data,
      const ledger::type::UrlResponse& response);

  void OnPublisherPanelInfo(
      uint64_t window_id,
      const ledger::type::VisitData& visit_data,
      const std::string& publisher_key,
      ledger::type::Result result,
      ledger::type::PublisherInfoPtr info);

  void OnMediaPublisherInfo(
      const std::string& user_name,
      ledger::PublisherInfoCallback callback,
      ledger::type::Result result,
      ledger::type::PublisherInfoPtr publisher_info);

  void SavePublisherInfo(
      uint64_t window_id,
      const std::string& user_name,
      ledger::PublisherInfoCallback callback,
      const std::string& data);

  void OnRedditSaved(
      ledger::type::Result result,
      ledger::type::PublisherInfoPtr publisher_info);

  static std::string GetUserNameFromUrl(const std::string& path);

  static std::string GetProfileUrl(const std::string& screen_name);

  static std::string GetUserId(const std::string& response);

  static std::string GetPublisherName(const std::string& response);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetProfileImageUrl(const std::string& response);

  void OnPageDataFetched(
      const std::string& user_name,
      ledger::PublisherInfoCallback callback,
      const ledger::type::UrlResponse& response);

  ledger::LedgerImpl* ledger_;  // NOT OWNED

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
