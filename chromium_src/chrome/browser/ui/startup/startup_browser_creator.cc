/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "brave/common/brave_switches.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"

#if defined(OS_WIN)
#include "brave/browser/microsoft_edge_protocol_util.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

class BraveStartupBrowserCreatorImpl final : public StartupBrowserCreatorImpl {
 public:
  BraveStartupBrowserCreatorImpl(const base::FilePath& cur_dir,
                                 const base::CommandLine& command_line,
                                 chrome::startup::IsFirstRun is_first_run);

  BraveStartupBrowserCreatorImpl(const base::FilePath& cur_dir,
                                 const base::CommandLine& command_line,
                                 StartupBrowserCreator* browser_creator,
                                 chrome::startup::IsFirstRun is_first_run);

  bool Launch(Profile* profile,
              const std::vector<GURL>& urls_to_open,
              bool process_startup,
              std::unique_ptr<LaunchModeRecorder> launch_mode_recorder);
};

BraveStartupBrowserCreatorImpl::BraveStartupBrowserCreatorImpl(
    const base::FilePath& cur_dir,
    const base::CommandLine& command_line,
    chrome::startup::IsFirstRun is_first_run)
    : StartupBrowserCreatorImpl(cur_dir, command_line, is_first_run) {}

BraveStartupBrowserCreatorImpl::BraveStartupBrowserCreatorImpl(
    const base::FilePath& cur_dir,
    const base::CommandLine& command_line,
    StartupBrowserCreator* browser_creator,
    chrome::startup::IsFirstRun is_first_run)
    : StartupBrowserCreatorImpl(cur_dir,
                                command_line,
                                browser_creator,
                                is_first_run) {}

// If the --tor command line flag was provided, switch the profile to Tor mode
// and then call the original Launch method.
//
// Note that if the --tor switch is used together with --silent-launch, Tor
// won't be launched.
bool BraveStartupBrowserCreatorImpl::Launch(
    Profile* profile,
    const std::vector<GURL>& urls_to_open,
    bool process_startup,
    std::unique_ptr<LaunchModeRecorder> launch_mode_recorder) {
  std::vector<GURL> revised_urls_to_open = urls_to_open;
#if defined(OS_WIN)
  for (const std::wstring& arg : command_line_.GetArgs()) {
    // Fetch url from command line args if it includes microsoft-edge protocol
    // and url is delivered.
    absl::optional<GURL> url = GetURLFromMSEdgeProtocol(arg);
    if (!url)
      continue;
    if (url->is_valid())
      revised_urls_to_open.push_back(std::move(*url));
  }
#endif

#if BUILDFLAG(ENABLE_TOR)
  if (StartupBrowserCreatorImpl::command_line_.HasSwitch(switches::kTor)) {
    LOG(INFO) << "Switching to Tor profile and starting Tor service.";
    profile = TorProfileManager::GetInstance().GetTorProfile(profile);

    // Call GetURLsFromCommandLine so that if one runs
    //   brave-browser --tor "? search query"
    // the search query is not passed to the default search engine of the
    // regular profile.
    const std::vector<GURL>& my_urls_to_open =
        GetURLsFromCommandLine(command_line_, cur_dir_, profile);
    return StartupBrowserCreatorImpl::Launch(profile, my_urls_to_open,
                                             process_startup,
                                             std::move(launch_mode_recorder));
  }
#endif

  return StartupBrowserCreatorImpl::Launch(profile, revised_urls_to_open,
                                           process_startup,
                                           std::move(launch_mode_recorder));
}

#define StartupBrowserCreatorImpl BraveStartupBrowserCreatorImpl
#include "../../../../../../chrome/browser/ui/startup/startup_browser_creator.cc"  // NOLINT
#undef StartupBrowserCreatorImpl
