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

bool ShouldUseFileTypeProgId(std::wstring_view ext) {
  return (ext == L".pdf" || ext == L".svg");
}

bool IsBrowserProgId(std::wstring_view prog_id) {
  const wchar_t* browser_prefix = install_static::GetBrowserProgIdPrefix();
  const wchar_t* pdf_prefix = install_static::GetPDFProgIdPrefix();
  return (browser_prefix && prog_id.starts_with(browser_prefix)) ||
         (pdf_prefix && prog_id.starts_with(pdf_prefix));
}

}  // namespace installer
