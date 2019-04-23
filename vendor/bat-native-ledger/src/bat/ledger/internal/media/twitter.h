/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_TWITTER_H_
#define BRAVELEDGER_MEDIA_TWITTER_H_

#include <map>
#include <string>

#include "base/gtest_prod_util.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_media {

class MediaTwitter : public ledger::LedgerCallbackHandler {
 public:
  explicit MediaTwitter(bat_ledger::LedgerImpl* ledger);

  ~MediaTwitter() override;

  void SaveMediaInfo(const std::map<std::string, std::string>& data,
                     ledger::SaveMediaInfoCallback callback);

 private:
  static std::string GetProfileURL(const std::string& screen_name);

  static std::string GetProfileImageURL(const std::string& screen_name);

  static std::string GetPublisherKey(const std::string& key);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_TWITTER_H_
