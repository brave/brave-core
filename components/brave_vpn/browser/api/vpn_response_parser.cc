/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/api/vpn_response_parser.h"

#include <optional>

#include "base/json/json_reader.h"
#include "base/logging.h"

namespace brave_vpn {

std::string ParseSubscriberCredentialFromJson(base::Value records_v,
                                              std::string* error) {
  if (!records_v.is_dict()) {
    VLOG(1) << __func__ << "Invalid response, could not parse JSON.";
    return "";
  }
  const auto& dict = records_v.GetDict();
  const auto* subscriber_credential = dict.FindString("subscriber-credential");
  const auto* error_title = dict.FindString("error-title");
  if (error_title && error) {
    *error = *error_title;
  }
  return subscriber_credential == nullptr ? "" : *subscriber_credential;
}

}  // namespace brave_vpn
