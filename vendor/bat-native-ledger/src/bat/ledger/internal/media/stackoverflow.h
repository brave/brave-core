/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_STACKOVERFLOW_H_
#define BRAVELEDGER_MEDIA_STACKOVERFLOW_H_

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

class StackOverflow : public ledger::LedgerCallbackHandler {
 public:
  explicit StackOverflow(bat_ledger::LedgerImpl* ledger);

  void SaveMediaInfo(
      const std::map<std::string, std::string>& data,
      ledger::PublisherInfoCallback callback);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::VisitData& visit_data);
  ~StackOverflow() override;

 private:
  void OnMediaPublisherActivity(
      ledger::Result result,
      ledger::PublisherInfoPtr info,
      uint64_t window_id,
      const ledger::VisitData& visit_data,
      const std::string& media_key,
      const std::string& user_id,
      const std::string& publisher_name,
      const std::string& profile_url,
      const std::string& profile_image);

  void FetchDataFromUrl(
      const std::string& url,
      braveledger_media::FetchDataFromUrlCallback callback);

  void OnPostPath(uint64_t window_id,
      const ledger::VisitData& visit_data,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnUserPath(uint64_t window_id,
    const ledger::VisitData& visit_data,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void OnSaveMediaVisit(
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void SavePublisherInfo(
      const std::string& media_key,
      const std::string& user_id,
      const std::string& publisher_name,
      const std::string& profile_url,
      const std::string& profile_image,
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

  void PostPath(
    uint64_t window_id,
    const ledger::VisitData& visit_data);

  void UserPath(
    uint64_t window_id,
    const ledger::VisitData& visit_data);

  void OnMediaActivityError(
      uint64_t window_id);

  void OnMediaPublisherInfo(
      ledger::Result result,
      ledger::PublisherInfoPtr info,
      uint64_t window_id,
      const std::string& media_key,
      const std::string& user_id,
      const std::string& publisher_name,
      const std::string& profile_url,
      const std::string& profile_image,
      ledger::PublisherInfoCallback callback);

  void OnMetaDataGet(
        ledger::PublisherInfoCallback callback,
        int response_status_code,
        const std::string& response,
        const std::map<std::string, std::string>& headers);

  static std::string GetUserNameFromURL(const std::string& path);

  static std::string GetUserName(const std::string& json_string);

  static std::string GetMediaKey(const std::string& user_name);

  static std::string GetUserId(const std::string& json_string);

  static std::string GetPublisherName(const std::string& json_string);

  static std::string GetProfileURL(const std::string& user_name);

  static std::string GetProfileAPIURL(const std::string& user_name);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetProfileImageURL(const std::string& json_string);

  static std::string GetIdFromURL(const std::string& path);

  static std::string GetAPIURLForPostId(const std::string&  post_id);

  static std::string GetAPIURLForUserId(const std::string&  user_id);

  static bool IsExcludedPath(const std::string& path);

  static bool GetJSONStringValue(const std::string& key,
      const std::string& json_string,
      std::string* result);

  static bool GetJSONIntValue(const std::string& key,
      const std::string& json_string,
      int64_t* result);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};
}  // namespace braveledger_media
#endif
