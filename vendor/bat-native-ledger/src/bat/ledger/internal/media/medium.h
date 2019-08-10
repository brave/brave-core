/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_MEDIUM_H_
#define BRAVELEDGER_MEDIA_MEDIUM_H_

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

class Medium : public ledger::LedgerCallbackHandler {
 public:
  explicit Medium(bat_ledger::LedgerImpl* ledger);

  ~Medium() override;

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
      const std::string& user_id,
      const std::string& publisher_name,
      ledger::PublisherInfoCallback callback,
      const std::string& data);

  void OnMediumSaved(
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

  static std::string GetUserNameFromUrl(const std::string& path);

  static std::string GetProfileUrl(const std::string& screen_name);

  static std::string GetUserId(const std::string& response,
                                const std::string& user_name);

  static std::string GetPublisherName(const std::string& response,
                                      const std::string& user_name);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetProfileImageUrl(const std::string& response);

  static std::string GetMediaKey(const std::string& screen_name);

  void OnPageDataFetched(
      const std::string& user_name,
      ledger::PublisherInfoCallback callback,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_MEDIUM_H_