/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_VIMEO_H_
#define BRAVELEDGER_MEDIA_VIMEO_H_

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

class Vimeo {
 public:
  explicit Vimeo(bat_ledger::LedgerImpl* ledger);

  ~Vimeo();

  void ProcessMedia(const std::map<std::string, std::string>& parts);

  static std::string GetLinkType(const std::string& url);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::VisitData& visit_data);

 private:
  static std::string GetVideoUrl(const std::string& video_id);

  static std::string GetMediaKey(const std::string& video_id,
                                 const std::string& type);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetIdFromVideoPage(const std::string& data);

  static std::string GenerateFaviconUrl(const std::string& id);

  static std::string GetNameFromVideoPage(const std::string& data);

  static std::string GetUrlFromVideoPage(const std::string& data);

  static bool AllowedEvent(const std::string& event);

  static uint64_t GetDuration(const ledger::MediaEventInfo& old_event,
                              const ledger::MediaEventInfo& new_event);

  static bool IsExcludedPath(const std::string& path);

  static std::string GetIdFromPublisherPage(const std::string& data);

  static std::string GetNameFromPublisherPage(const std::string& data);

  static std::string GetVideoIdFromVideoPage(const std::string& data);

  void FetchDataFromUrl(
    const std::string& url,
    ledger::LoadURLCallback callback);

  void OnMediaActivityError(uint64_t window_id = 0);

  void OnEmbedResponse(
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    const ledger::UrlResponse& response);

  void OnPublisherPage(
    const std::string& media_key,
    const std::string& publisher_url,
    const std::string& publisher_name,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    const ledger::UrlResponse& response);

  void OnUnknownPage(
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    const ledger::UrlResponse& response);

  void OnPublisherPanleInfo(
    const std::string& media_key,
    uint64_t window_id,
    const std::string& publisher_url,
    const std::string& publisher_name,
    const std::string& user_id,
    ledger::Result result,
    ledger::PublisherInfoPtr info);

  void GetPublisherPanleInfo(
    const std::string& media_key,
    uint64_t window_id,
    const std::string& publisher_url,
    const std::string& publisher_key,
    const std::string& publisher_name,
    const std::string& user_id);

  void OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const ledger::MediaEventInfo& event_info,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info);

  void OnPublisherVideoPage(
    const std::string& media_key,
    ledger::MediaEventInfo event_info,
    const ledger::UrlResponse& response);

  void SavePublisherInfo(
    const std::string& media_key,
    const uint64_t duration,
    const std::string& user_id,
    const std::string& publisher_name,
    const std::string& publisher_url,
    const uint64_t window_id,
    const std::string& publisher_key = "",
    const std::string& publisher_favicon = "");

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, ledger::MediaEventInfo> events;

  // For testing purposes
  friend class VimeoTest;
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetVideoUrl);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetMediaKey);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetPublisherKey);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetIdFromVideoPage);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GenerateFaviconUrl);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetNameFromVideoPage);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetUrlFromVideoPage);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, AllowedEvent);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetDuration);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, IsExcludedPath);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetIdFromPublisherPage);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetNameFromPublisherPage);
  FRIEND_TEST_ALL_PREFIXES(VimeoTest, GetVideoIdFromVideoPage);
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_VIMEO_H_
