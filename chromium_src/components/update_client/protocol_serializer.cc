/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define MakeProtocolRequest MakeProtocolRequest_ChromiumImpl
#include "../../../../components/update_client/protocol_serializer.cc"
#undef MakeProtocolRequest

namespace update_client {

protocol_request::Request MakeProtocolRequest(
    const std::string& session_id,
    const std::string& prod_id,
    const std::string& browser_version,
    const std::string& lang,
    const std::string& channel,
    const std::string& os_long_name,
    const std::string& download_preference,
    const base::flat_map<std::string, std::string>& additional_attributes,
    const std::map<std::string, std::string>* updater_state_attributes,
    std::vector<protocol_request::App> apps) {
  protocol_request::Request request = MakeProtocolRequest_ChromiumImpl(
      session_id, prod_id, browser_version, lang, channel, os_long_name,
      download_preference, additional_attributes, updater_state_attributes,
      std::move(apps));
#if defined(OS_APPLE) && !defined(OS_IOS)
  // temporary workaround for google update server not returning widevine
  // for arm64
  request.arch = "x64";
  request.nacl_arch = "x64";
  request.os.arch = "x64";
#endif
  return request;
}

}  // namespace update_client
