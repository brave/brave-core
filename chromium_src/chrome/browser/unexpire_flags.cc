/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/version_info.h"

#define IsFlagExpired IsFlagExpired_ChromiumImpl
#include "../../../../chrome/browser/unexpire_flags.cc"
#undef IsFlagExpired

namespace flags {

bool IsFlagExpired(const flags_ui::FlagsStorage* storage,
                   const char* internal_name) {
  // Enable sidebar feature only for nightly/development.
  version_info::Channel channel = chrome::GetChannel();
  if (base::LowerCaseEqualsASCII("sidebar", internal_name) &&
      (channel == version_info::Channel::STABLE ||
       channel == version_info::Channel::BETA)) {
    return true;
  }

  return IsFlagExpired_ChromiumImpl(storage, internal_name);
}

}  // namespace flags
