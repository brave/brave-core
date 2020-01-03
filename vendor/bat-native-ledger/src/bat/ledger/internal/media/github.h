/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_GITHUB_H_
#define BRAVELEDGER_MEDIA_GITHUB_H_

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

class GitHub : public ledger::LedgerCallbackHandler {
 public:
  explicit GitHub(bat_ledger::LedgerImpl* ledger);

  static std::string GetLinkType(const std::string& url);

  void SaveMediaInfo(
      const std::map<std::string, std::string>& data,
      ledger::PublisherInfoCallback callback);

  void ProcessActivityFromUrl(
      const ledger::VisitData& visit_data,
      ledger::GetPublisherActivityFromUrlCallback callback);

  void ProcessMedia(
      const std::map<std::string, std::string> parts,
      const ledger::VisitData& visit_data,
      ledger::GetPublisherActivityFromUrlCallback callback);

  ~GitHub() override;

 private:
  void OnMediaPublisherActivity(
      ledger::Result result,
      ledger::PublisherInfoPtr info,
      const ledger::VisitData& visit_data,
      const std::string& media_key,
      ledger::GetPublisherActivityFromUrlCallback callback);

  void FetchDataFromUrl(
      const std::string& url,
      braveledger_media::FetchDataFromUrlCallback callback);

  void OnUserPage(
      const uint64_t duration,
      const ledger::VisitData& visit_data,
      ledger::GetPublisherActivityFromUrlCallback callback,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnSaveMediaVisit(
      ledger::GetPublisherActivityFromUrlCallback callback,
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void SavePublisherInfo(
      const uint64_t duration,
      const std::string& user_id,
      const std::string& user_name,
      const std::string& publisher_name,
      const std::string& profile_picture,
      ledger::PublisherInfoCallback callback);

  void GetPublisherPanelInfo(
      const ledger::VisitData& visit_data,
      const std::string& publisher_key,
      ledger::GetPublisherActivityFromUrlCallback callback);

  void OnPublisherPanelInfo(
      const ledger::VisitData& visit_data,
      const std::string& publisher_key,
      ledger::GetPublisherActivityFromUrlCallback callback,
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void OnMediaActivityError(
      ledger::GetPublisherActivityFromUrlCallback callback);

void OnMetaDataGet(
      ledger::PublisherInfoCallback callback,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

void OnMediaPublisherInfo(
    const std::string& user_id,
    const std::string& screen_name,
    const std::string& publisher_name,
    const std::string& profile_picture,
    ledger::PublisherInfoCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info);

  static std::string GetUserNameFromURL(const std::string& path);

  static std::string GetUserName(const std::string& json_string);

  static std::string GetMediaKey(const std::string& user_name);

  static std::string GetUserId(const std::string& json_string);

  static std::string GetPublisherName(const std::string& json_string);

  static std::string GetProfileURL(const std::string& user_name);

  static std::string GetProfileAPIURL(const std::string& user_name);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetProfileImageURL(const std::string& json_string);

  static bool IsExcludedPath(const std::string& path);

  static bool GetJSONStringValue(const std::string& key,
      const std::string& json_string,
      std::string* result);

  static bool GetJSONIntValue(const std::string& key,
      const std::string& json_string,
      int64_t* result);

  // For testing purposes
  friend class MediaGitHubTest;
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetLinkType);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileURL);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileAPIURL);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileImageURL);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetPublisherKey);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetMediaKey);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserNameFromURL);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserName);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserId);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetPublisherName);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetJSONStringValue);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetJSONIntValue);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};
}  // namespace braveledger_media
#endif
