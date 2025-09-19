/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_UPDATE_CLIENT_TEST_UTIL_H_
#define BRAVE_COMPONENTS_UPDATE_CLIENT_TEST_UTIL_H_

#include "components/update_client/protocol_serializer.h"

namespace update_client {

// This function checks two things. First, that the serializer faithfully
// encodes the necessary data for update requests. Second, that it does not
// encode the following fields, which could be used to fingerprint users:
//  - hw[*]
//  - apps[*].lang
//  - apps[*].events[*].download_time_ms
bool StripsPrivacySensitiveData(const ProtocolSerializer& serializer);

}  // namespace update_client

#endif  // BRAVE_COMPONENTS_UPDATE_CLIENT_TEST_UTIL_H_
