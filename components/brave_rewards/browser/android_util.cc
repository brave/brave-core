/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/browser/android_util.h"
#include "brave/common/brave_channel_info.h"

namespace android_util {

ledger::ClientInfoPtr GetAndroidClientInfo() {
  auto info = ledger::ClientInfo::New();
  info->platform = ledger::Platform::ANDROID_R;
  info->os = ledger::OperatingSystem::UNDEFINED;
  info->channel = brave::GetChannelName();
  return info;
}

std::string ParseClaimPromotionResponse(const std::string& response) {
  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return "";
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return "";
  }

  auto* nonce = dictionary->FindKey("nonce");
  if (!nonce || !nonce->is_string()) {
    return "";
  }

  return nonce->GetString();
}

}  // namespace android_util
