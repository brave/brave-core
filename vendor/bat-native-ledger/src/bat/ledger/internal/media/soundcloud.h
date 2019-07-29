/* Copyright  2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_SOUNDCLOUD_H_
#define BRAVELEDGER_MEDIA_SOUNDCLOUD_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/values.h"
#include "base/optional.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/media/helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_media {

class SoundCloud : public ledger::LedgerCallbackHandler {
 public:
  explicit SoundCloud(bat_ledger::LedgerImpl* ledger);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::VisitData& visit_data);
  ~SoundCloud() override;

 private:
  void OnMediaPublisherActivity(
      ledger::Result result,
      ledger::PublisherInfoPtr info,
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const std::string& media_key);

  void FetchDataFromUrl(
      const std::string& url,
      braveledger_media::FetchDataFromUrlCallback callback);

  void OnUserPage(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnSaveMediaVisit(
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void SavePublisherInfo(
      const std::string& user_id,
      const std::string& user_url,
      const std::string& publisher_name,
      const std::string& profile_picture,
      const uint64_t window_id,
      ledger::PublisherInfoCallback callback);

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

  void OnMediaActivityError(
      uint64_t window_id);

  static std::string GetUserJSON(const std::string& response);

  static std::string GetUserName(const std::string& json_string);

  static std::string GetBaseURL(const std::string& path);

  static std::string GetMediaKey(const std::string& user_name);

  static std::string GetUserId(const std::string& json_string);

  static std::string GetPublisherName(const std::string& json_string);

  static std::string GetProfileURL(const std::string& user_url);

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
  friend class MediaSoundCloudTest;
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetUserJSON);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetUserName);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetBaseURL);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetUserId);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetPublisherName);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetProfileURL);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetPublisherKey);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetProfileImageURL);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetJSONStringValue);
  FRIEND_TEST_ALL_PREFIXES(MediaSoundCloudTest, GetJSONIntValue);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};
}  // namespace braveledger_media
#endif
