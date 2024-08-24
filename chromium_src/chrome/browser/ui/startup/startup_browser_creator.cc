/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

#ifdef LaunchModeRecorder
static_assert(false,
              "Replace the use of OldLaunchModeRecorder with "
              "LaunchModeRecorder, and remove this assert.");
#endif  // #ifdef LaunchModeRecorder

class BraveStartupBrowserCreatorImpl final : public StartupBrowserCreatorImpl {
 public:
  BraveStartupBrowserCreatorImpl(const base::FilePath& cur_dir,
                                 const base::CommandLine& command_line,
                                 chrome::startup::IsFirstRun is_first_run);

  BraveStartupBrowserCreatorImpl(const base::FilePath& cur_dir,
                                 const base::CommandLine& command_line,
                                 StartupBrowserCreator* browser_creator,
                                 chrome::startup::IsFirstRun is_first_run);

  void Launch(Profile* profile,
              chrome::startup::IsProcessStartup process_startup,
              std::unique_ptr<OldLaunchModeRecorder> launch_mode_recorder,
              bool restore_tabbed_browser);
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
void BraveStartupBrowserCreatorImpl::Launch(
    Profile* profile,
    chrome::startup::IsProcessStartup process_startup,
    std::unique_ptr<OldLaunchModeRecorder> launch_mode_recorder,
    bool restore_tabbed_browser) {
#if BUILDFLAG(ENABLE_TOR)
  if (StartupBrowserCreatorImpl::command_line_->HasSwitch(switches::kTor)) {
    // Call StartupBrowserCreatorImpl::Launch() with the Tor profile so that if
    // one runs brave-browser --tor "? search query" the search query is not
    // passed to the default search engine of the regular profile.
    LOG(INFO) << "Switching to Tor profile and starting Tor service.";
    profile = TorProfileManager::GetInstance().GetTorProfile(profile);
  }
#endif

  StartupBrowserCreatorImpl::Launch(profile, process_startup,
                                    std::move(launch_mode_recorder),
                                    restore_tabbed_browser);
}

#define StartupBrowserCreatorImpl BraveStartupBrowserCreatorImpl
#include "src/chrome/browser/ui/startup/startup_browser_creator.cc"
#undef StartupBrowserCreatorImpl
