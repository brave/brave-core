/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <utility>

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher_internal.h"
#include "brave/components/brave_vpn/common/v2/branding_buildflags.h"

namespace brave_vpn::v2::internal {
namespace {
// The agent bundle is dropped into the browser framework's Helpers directory.
constexpr char kHelpersDirectory[] = "Helpers";
}  // namespace

base::FilePath GetValidAgentPath() {
  const base::FilePath framework_path = base::apple::FrameworkBundlePath();
  if (framework_path.empty()) {
    LOG(ERROR) << "No framework bundle path, cannot locate the VPN agent";
    return {};
  }
  const base::FilePath agent_path =
      framework_path.AppendASCII(kHelpersDirectory)
          .AppendASCII(BUILDFLAG(VPN_AGENT_APP_NAME));
  if (!base::PathExists(agent_path)) {
    LOG(ERROR) << "VPN agent not found at " << agent_path;
    return {};
  }
  return agent_path;
}

void LaunchAgentProcess(AgentLauncher::LaunchFailureCallback failure_callback,
                        const base::FilePath& agent_path) {
  NSURL* bundle_url = base::apple::FilePathToNSURL(agent_path);
  if (!bundle_url) {
    std::move(failure_callback).Run(AgentLauncher::LaunchError::kAppNotFound);
    return;
  }

  // Never posix_spawn the executable inside the bundle. Going through
  // LaunchServices is what makes launchd the agent's parent rather than this
  // browser, and what gives the agent its own activation policy and TCC
  // identity. It is also idempotent by bundle identifier: if an instance is
  // already running, that instance is returned instead of a second one being
  // started.
  NSWorkspaceOpenConfiguration* configuration =
      [NSWorkspaceOpenConfiguration configuration];
  configuration.activates = NO;
  configuration.addsToRecentItems = NO;
  configuration.createsNewApplicationInstance = NO;
  configuration.hides = YES;

  // The completion handler runs on an arbitrary queue, which is fine: the
  // failure callback is already bound to the sequence that called this
  // function.
  __block AgentLauncher::LaunchFailureCallback block_callback =
      std::move(failure_callback);

  [NSWorkspace.sharedWorkspace
      openApplicationAtURL:bundle_url
             configuration:configuration
         completionHandler:^(NSRunningApplication* application,
                             NSError* error) {
           if (error) {
             LOG(ERROR) << "Failed to open the VPN agent bundle: "
                        << base::SysNSStringToUTF8(error.localizedDescription);
             std::move(block_callback)
                 .Run(AgentLauncher::LaunchError::kLaunchFailed);
           }
         }];
}

}  // namespace brave_vpn::v2::internal
