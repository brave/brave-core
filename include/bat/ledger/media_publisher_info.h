/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_MEDIA_PUBLISHER_INFO_HANDLER_
#define BAT_LEDGER_MEDIA_PUBLISHER_INFO_HANDLER_

#include "bat/ledger/export.h"

namespace ledger {

LEDGER_EXPORT struct TwitchEventInfo {
  TwitchEventInfo();
  TwitchEventInfo(const TwitchEventInfo&);
  ~TwitchEventInfo();

  std::string event_;
  std::string time_;
  std::string status_;
};

LEDGER_EXPORT struct MediaPublisherInfo {
  MediaPublisherInfo(const std::string& publisher_id);
  MediaPublisherInfo(const MediaPublisherInfo& info);
  ~MediaPublisherInfo();

  static std::unique_ptr<MediaPublisherInfo> FromJSON(const std::string& json);
  const std::string ToJSON() const;

  std::string publisher_id_;
  std::string publisherName_;
  std::string publisherURL_;
  std::string favIconURL_;
  std::string channelName_;
  std::string publisher_;
  TwitchEventInfo twitchEventInfo_;
};

}  // namespace ledger

#endif  // BAT_LEDGER_MEDIA_PUBLISHER_INFO_HANDLER_
