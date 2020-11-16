/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/rappor/log_uploader.h"

#define Start Start_ChromiumImpl
#include "../../../../components/rappor/log_uploader.cc"
#undef Start

namespace rappor {

void LogUploader::Start() {
  return;
}

}  // namespace rappor
