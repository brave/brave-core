/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_TWITCH_H_
#define BRAVELEDGER_MEDIA_TWITCH_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/media/helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_media {

class MediaTwitch : public ledger::LedgerCallbackHandler {
 public:
  explicit MediaTwitch(bat_ledger::LedgerImpl* ledger);

  ~MediaTwitch() override;

  void ProcessMedia(const std::map<std::string, std::string>& parts,
                    const ledger::VisitData& visit_data);

 private:
  static std::string GetMediaIdFromParts(
      const std::map<std::string, std::string>& parts);

  static std::string GetMediaURL(const std::string& mediaId);

  static std::string GetTwitchStatus(
    const ledger::TwitchEventInfo& old_event,
    const ledger::TwitchEventInfo& new_event);

  static uint64_t GetTwitchDuration(
    const ledger::TwitchEventInfo& old_event,
    const ledger::TwitchEventInfo& new_event);

  void OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const ledger::TwitchEventInfo& twitch_info,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback);

  void OnEmbedResponse(
    const uint64_t duration,
    const std::string& media_key,
    const std::string& media_url,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, ledger::TwitchEventInfo> twitch_events;
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_TWITCH_H_
