/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/version_info/channel.h"
#include "brave/components/constants/url_constants.h"  // for kDownloadBraveUrl
#include "chrome/common/channel_info.h"
#include "content/public/browser/page_navigator.h"
#include "url/gurl.h"

namespace {

const char* BraveGetUpdateUrlSuffix(version_info::Channel channel) {
  switch (channel) {
    case version_info::Channel::CANARY:
      return "-nightly";
    case version_info::Channel::DEV:
      return "-dev";
    case version_info::Channel::BETA:
      return "-beta";
    case version_info::Channel::UNKNOWN:
    case version_info::Channel::STABLE:
      return "";
  }
}

GURL BraveGetUpdateUrl() {
  return GURL(std::string(kDownloadBraveUrl) +
              BraveGetUpdateUrlSuffix(chrome::GetChannel()));
}

}  // namespace

#define OpenURLParams(URL, ...) OpenURLParams(BraveGetUpdateUrl(), __VA_ARGS__)

#include "src/chrome/browser/ui/dialogs/outdated_upgrade_bubble.cc"
#undef OpenURLParams
