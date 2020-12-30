/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_VIMEO_H_
#define BRAVELEDGER_MEDIA_VIMEO_H_

#include <stdint.h>

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

class Vimeo {
 public:
  explicit Vimeo(ledger::LedgerImpl* ledger);

  ~Vimeo();

  void ProcessMedia(const base::flat_map<std::string, std::string>& parts);

  static std::string GetLinkType(const std::string& url);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::type::VisitData& visit_data);

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

  static uint64_t GetDuration(const ledger::type::MediaEventInfo& old_event,
                              const ledger::type::MediaEventInfo& new_event);

  static bool IsExcludedPath(const std::string& path);

  static std::string GetIdFromPublisherPage(const std::string& data);

  static std::string GetNameFromPublisherPage(const std::string& data);

  static std::string GetVideoIdFromVideoPage(const std::string& data);

  void FetchDataFromUrl(
    const std::string& url,
    ledger::client::LoadURLCallback callback);

  void OnMediaActivityError(uint64_t window_id = 0);

  void OnEmbedResponse(
    const ledger::type::VisitData& visit_data,
    const uint64_t window_id,
    const ledger::type::UrlResponse& response);

  void OnPublisherPage(
    const std::string& media_key,
    const std::string& publisher_url,
    const std::string& publisher_name,
    const ledger::type::VisitData& visit_data,
    const uint64_t window_id,
    const ledger::type::UrlResponse& response);

  void OnUnknownPage(
    const ledger::type::VisitData& visit_data,
    const uint64_t window_id,
    const ledger::type::UrlResponse& response);

  void OnPublisherPanleInfo(
    const std::string& media_key,
    uint64_t window_id,
    const std::string& publisher_url,
    const std::string& publisher_name,
    const std::string& user_id,
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr info);

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
    const ledger::type::MediaEventInfo& event_info,
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr publisher_info);

  void OnPublisherVideoPage(
    const std::string& media_key,
    ledger::type::MediaEventInfo event_info,
    const ledger::type::UrlResponse& response);

  void SavePublisherInfo(
    const std::string& media_key,
    const uint64_t duration,
    const std::string& user_id,
    const std::string& publisher_name,
    const std::string& publisher_url,
    const uint64_t window_id,
    const std::string& publisher_key = "",
    const std::string& publisher_favicon = "");

  ledger::LedgerImpl* ledger_;  // NOT OWNED
  base::flat_map<std::string, ledger::type::MediaEventInfo> events;

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
