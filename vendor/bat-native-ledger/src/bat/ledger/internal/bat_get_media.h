/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_GET_MEDIA_H_
#define BRAVELEDGER_BAT_GET_MEDIA_H_

#include <string>
#include <map>
#include <memory>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/media/twitch.h"
#include "bat/ledger/internal/media/twitter.h"
#include "bat/ledger/internal/media/youtube.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_bat_get_media {

class BatGetMedia {
 public:
  explicit BatGetMedia(bat_ledger::LedgerImpl* ledger);

  ~BatGetMedia();

  static std::string GetLinkType(const std::string& url,
                                 const std::string& first_party_url,
                                 const std::string& referrer);

  void ProcessMedia(const std::map<std::string, std::string>& parts,
                    const std::string& type,
                    const ledger::VisitData& visit_data);

  void GetMediaActivityFromUrl(uint64_t windowId,
                               const ledger::VisitData& visit_data,
                               const std::string& type,
                               const std::string& publisher_blob);

  void SaveMediaInfo(const std::string& type,
                     const std::map<std::string, std::string>& data,
                     ledger::SaveMediaInfoCallback callback);

 private:
  void OnMediaActivityError(const ledger::VisitData& visit_data,
                          const std::string& type,
                          uint64_t windowId);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<braveledger_media::MediaYouTube> media_youtube_;
  std::unique_ptr<braveledger_media::MediaTwitch> media_twitch_;
  std::unique_ptr<braveledger_media::MediaTwitter> media_twitter_;
};

}  // namespace braveledger_bat_get_media

#endif  // BRAVELEDGER_BAT_GET_MEDIA_H_
