/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_IMGUR_H_
#define BRAVELEDGER_MEDIA_IMGUR_H_

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

class Imgur : public ledger::LedgerCallbackHandler {
 public:
  explicit Imgur(bat_ledger::LedgerImpl* ledger);

  ~Imgur() override;

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::VisitData& visit_data);

  void SaveMediaInfo(
      const std::map<std::string, std::string>& data,
      ledger::PublisherInfoCallback callback);


 private:
  void OnUserActivity(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

	void OnMediaPublisherActivity(
			ledger::Result result,
			ledger::PublisherInfoPtr publisher_info,
			uint64_t window_id,
			const ledger::VisitData& visit_data,
			const std::string& media_key,
			const std::string& user_name,
			const std::string& publisher_name);

  void UserPath(
      uint64_t window_id,
      const ledger::VisitData& visit_data);

  void ImagePath(
      uint64_t window_id,
      const ledger::VisitData& visit_data);

  void FetchUserNameFromMedia(
      uint64_t window_id,
      const ledger::VisitData& visit_data);

  void FetchDataFromUrl(
      const std::string& url,
      braveledger_media::FetchDataFromUrlCallback callback);

  void OnUserPage(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnImagePath(
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnSaveMediaVisit(
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void SavePublisherInfo(
      uint64_t window_id,
      const std::string& user_name,
      const std::string& publisher_name,
      ledger::PublisherInfoCallback callback,
      const std::string& data);

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
      const ledger::VisitData& visit_data,
      uint64_t window_id);

void OnMetaDataGet(
      ledger::PublisherInfoCallback callback,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

void OnImgurSaved(
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info);

void OnMediaPublisherInfo(
		const std::string& user_name,
		ledger::PublisherInfoCallback callback,
		ledger::Result result,
		ledger::PublisherInfoPtr publisher_info);

  static std::string GetUserNameFromUrl(const std::string& path);

  static std::string GetUserName(const std::string& json_string);

  static std::string GetMediaKey(const std::string& user_name);

  static std::string GetUserId(const std::string& json_string);

  static std::string GetPublisherName(const std::string& response,
                                      const std::string& user_name);

  static std::string GetPublisherNameFromImage(const std::string& response);

  static std::string GetProfileUrl(const std::string& user_name);

  //static std::string GetProfileAPIURL(const std::string& user_name);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetProfileImageUrl(const std::string& json_string);

  void OnPageDataFetched(const std::string& user_name,
                         ledger::PublisherInfoCallback callback,
                         int response_status_code,
                         const std::string& response,
                         const std::map<std::string, std::string>& headers);


  static bool IsExcludedPath(const std::string& path);

  static bool GetJSONStringValue(const std::string& key,
      const std::string& json_string,
      std::string* result);

  static bool GetJSONIntValue(const std::string& key,
      const std::string& json_string,
      int64_t* result);

  // For testing purposes
  //friend class MediaGitHubTest;
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileURL);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileAPIURL);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileImageURL);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetPublisherKey);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetMediaKey);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserNameFromURL);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserName);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserId);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetPublisherName);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetJSONStringValue);
  //FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetJSONIntValue);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};
}  // namespace braveledger_media
#endif
