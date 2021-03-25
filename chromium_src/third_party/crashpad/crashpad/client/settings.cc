/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/crashpad/crashpad/client/settings.h"

#define GetClientID GetClientID_ChromiumImpl
#include "../../../../../../third_party/crashpad/crashpad/client/settings.cc"
#undef GetClientID

namespace crashpad {

bool Settings::GetClientID(UUID* client_id) {
  if (!GetClientID_ChromiumImpl(client_id))
    return false;

  client_id->InitializeToZero();
  return true;
}

}  // namespace crashpad
