/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/installer/util/brave_shell_util.h"

#include "base/notreached.h"
#include "chrome/install_static/install_util.h"
#include "components/version_info/channel.h"

namespace installer {

std::wstring GetProgIdForFileType() {
  switch (install_static::GetChromeChannel()) {
    case version_info::Channel::STABLE:
      return L"BraveFile";
    case version_info::Channel::BETA:
      return L"BraveBFile";
    case version_info::Channel::DEV:
      return L"BraveDFile";
    case version_info::Channel::CANARY:
      return L"BraveSSFile";
    default:
      break;
  }
  // install_static::GetChromeChannel() only gives above four types
  // for official build. And we don't support installer build for
  // unofficial build.
  NOTREACHED();
}

bool ShouldUseFileTypeProgId(const std::wstring& ext) {
  return (ext == L".pdf" || ext == L".svg");
}

}  // namespace installer
