/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/startup_browser_creator.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
#include "brave/browser/ui/views/brave_origin/brave_origin_startup_view.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
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
              bool restore_tabbed_browser);

  void MaybeShowNonMilestoneUpdateToast(
      Browser* browser,
      const std::string& current_version_string) override {}
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
// This switch is primarily used for testing and is not the same as using the
// Tor browser. In particular, you will see some profile-wide network traffic
// not going through the tor proxy (e.g. adblock list updates, P3A).
//
// Note that if the --tor switch is used together with --silent-launch, Tor
// won't be launched.
void BraveStartupBrowserCreatorImpl::Launch(
    Profile* profile,
    chrome::startup::IsProcessStartup process_startup,
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
                                    restore_tabbed_browser);
}

#define StartupBrowserCreatorImpl BraveStartupBrowserCreatorImpl
#define Start(...) Start_ChromiumImpl(__VA_ARGS__)
#define ProcessCommandLineAlreadyRunning(...) \
  ProcessCommandLineAlreadyRunning_ChromiumImpl(__VA_ARGS__)
#include <chrome/browser/ui/startup/startup_browser_creator.cc>
#undef ProcessCommandLineAlreadyRunning
#undef Start
#undef StartupBrowserCreatorImpl

#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
namespace {

// Concrete delegate that wires up the startup dialog to real browser services.
class StartupDialogDelegate : public BraveOriginStartupView::Delegate {
 public:
  void OpenExternal(const GURL& url) override {
    platform_util::OpenExternal(url);
  }

  void AttemptExit() override { chrome::AttemptExit(); }

  void CreateSystemProfile(
      BraveOriginStartupView::ProfileCallback callback) override {
    g_browser_process->profile_manager()->CreateProfileAsync(
        ProfileManager::GetSystemProfilePath(), std::move(callback));
  }

  void CreateDefaultProfile(
      BraveOriginStartupView::ProfileCallback callback) override {
    g_browser_process->profile_manager()->CreateProfileAsync(
        g_browser_process->profile_manager()->GetLastUsedProfileDir(),
        std::move(callback));
  }
};

}  // namespace
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)

// For Brave Origin branded builds, intercept Start() to show a purchase
// validation dialog before any browser window or profile picker opens. Start()
// is only called externally from chrome_browser_main.cc, so the #define above
// only renames the definition (not external callers). When the dialog closes
// with a successful validation, the callback invokes Start_ChromiumImpl() to
// continue the normal startup flow.
bool StartupBrowserCreator::Start(const base::CommandLine& cmd_line,
                                  const base::FilePath& cur_dir,
                                  StartupProfileInfo profile_info,
                                  const Profiles& last_opened_profiles) {
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  if (BraveOriginStartupView::ShouldShowDialog(
          g_browser_process->local_state())) {
    // Capture first_run_tabs_ by value because `this` (the
    // StartupBrowserCreator) is destroyed by chrome_browser_main.cc
    // (browser_creator_.reset()) right after Start() returns.
    BraveOriginStartupView::Show(
        base::BindOnce(
            [](std::vector<GURL> first_run_tabs,
               const base::CommandLine& cmd_line, const base::FilePath& cur_dir,
               StartupProfileInfo profile_info,
               const Profiles& last_opened_profiles) {
              StartupBrowserCreator browser_creator;
              browser_creator.AddFirstRunTabs(first_run_tabs);
              browser_creator.Start_ChromiumImpl(cmd_line, cur_dir,
                                                 std::move(profile_info),
                                                 last_opened_profiles);
            },
            std::move(first_run_tabs_), cmd_line, cur_dir,
            std::move(profile_info), last_opened_profiles),
        std::make_unique<StartupDialogDelegate>());
    return true;
  }
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  return Start_ChromiumImpl(cmd_line, cur_dir, std::move(profile_info),
                            last_opened_profiles);
}

// For Brave Origin branded builds, block second-launch attempts while the
// startup dialog is showing. Without this, a second launch goes through
// ProcessCommandLineAlreadyRunning (not Start), bypassing the dialog guard
// and opening a browser window directly.
// static
void StartupBrowserCreator::ProcessCommandLineAlreadyRunning(
    const base::CommandLine& command_line,
    const base::FilePath& cur_dir,
    const StartupProfilePathInfo& profile_path_info) {
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  if (BraveOriginStartupView::IsShowing()) {
    return;
  }
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  ProcessCommandLineAlreadyRunning_ChromiumImpl(command_line, cur_dir,
                                                profile_path_info);
}
