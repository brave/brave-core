/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/startup_tab_provider.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"

#if defined(OS_WIN)
#include "brave/browser/microsoft_edge_protocol_util.h"
#endif

#define StartupTabProviderImpl ChromiumStartupTabProviderImpl
#include "src/chrome/browser/ui/startup/startup_tab_provider.cc"
#undef StartupTabProviderImpl

StartupTabs StartupTabProviderImpl::GetCommandLineTabs(
    const base::CommandLine& command_line,
    const base::FilePath& cur_dir,
    Profile* profile) const {
  StartupTabs result = ChromiumStartupTabProviderImpl::GetCommandLineTabs(
      command_line, cur_dir, profile);
#if defined(OS_WIN)
  for (const std::wstring& arg : command_line.GetArgs()) {
    // Fetch url from command line args if it includes microsoft-edge protocol
    // and url is delivered.
    absl::optional<GURL> url = GetURLFromMSEdgeProtocol(arg);
    if (!url)
      continue;
    if (url->is_valid())
      result.push_back(StartupTab(url.value()));
  }
#endif

  return result;
}
