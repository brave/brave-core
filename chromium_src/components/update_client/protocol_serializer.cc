/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/update_client/protocol_serializer.h"

#define BuildUpdateCheckExtraRequestHeaders \
  BuildUpdateCheckExtraRequestHeaders_ChromiumImpl
#include "../../../../components/update_client/protocol_serializer.cc"
#undef BuildUpdateCheckExtraRequestHeaders

namespace update_client {

base::flat_map<std::string, std::string> BuildUpdateCheckExtraRequestHeaders(
    const std::string& prod_id,
    const base::Version& browser_version,
    const std::vector<std::string>& ids,
    bool is_foreground) {
  auto headers = BuildUpdateCheckExtraRequestHeaders_ChromiumImpl(
      prod_id, browser_version, ids, is_foreground);
  headers.insert({"BraveServiceKey", BRAVE_SERVICES_KEY});
  return headers;
}

}  // namespace update_client
