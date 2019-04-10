/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_YOUTUBE_H_
#define BRAVELEDGER_MEDIA_YOUTUBE_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/media/helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_media {

class MediaYouTube : public ledger::LedgerCallbackHandler {
 public:
  explicit MediaYouTube(bat_ledger::LedgerImpl* ledger);

  ~MediaYouTube() override;

  void ProcessMedia(const std::map<std::string, std::string>& parts,
                    const ledger::VisitData& visit_data);

  static std::string GetLinkType(const std::string& url);

 private:

  static std::string GetMediaIdFromParts(
      const std::map<std::string, std::string>& parts);

  static uint64_t GetMediaDurationFromParts(
      const std::map<std::string, std::string>& data,
      const std::string& media_key);

  static std::string GetVideoUrl(const std::string& media_id);

  static std::string GetChannelUrl(const std::string& publisher_key);

  static std::string GetFavIconUrl(const std::string& data);

  static std::string GetChannelId(const std::string& data);

  static std::string GetPublisherName(const std::string& data);

  void OnMediaActivityError(const ledger::VisitData& visit_data,
                            uint64_t window_id);

  void OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const uint64_t duration,
    const ledger::VisitData& visit_data,
    uint64_t window_id,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void OnEmbedResponse(
    const uint64_t duration,
    const std::string& media_key,
    const std::string& media_url,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void OnPublisherPage(
    const uint64_t duration,
    const std::string& media_key,
    std::string publisher_url,
    std::string publisher_name,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void SavePublisherInfo(const uint64_t duration,
                         const std::string& media_key,
                         const std::string& publisher_url,
                         const std::string& publisher_name,
                         const ledger::VisitData& visit_data,
                         const uint64_t window_id,
                         const std::string& fav_icon,
                         const std::string& channel_id);

  void FetchDataFromUrl(const std::string& url,
                        braveledger_media::FetchDataFromUrlCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_YOUTUBE_H_
