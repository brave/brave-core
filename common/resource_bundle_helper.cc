/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/resource_bundle_helper.h"

#include <string>

#include "base/command_line.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "ui/base/resource/resource_bundle.h"

#if !BUILDFLAG(IS_IOS)
#include "chrome/common/chrome_paths.h"
#include "content/public/common/content_switches.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "base/apple/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "ui/base/resource/resource_bundle_android.h"
#endif

namespace {

#if !BUILDFLAG(IS_ANDROID)
base::FilePath GetResourcesPakFilePath() {
#if BUILDFLAG(IS_MAC)
  return base::apple::PathForFrameworkBundleResource("brave_resources.pak");
#else
  base::FilePath pak_path;
  base::PathService::Get(base::DIR_ASSETS, &pak_path);
  pak_path = pak_path.AppendASCII("brave_resources.pak");
  return pak_path;
#endif  // OS_MAC
}
#endif  // OS_ANDROID

#if !BUILDFLAG(IS_ANDROID)
base::FilePath GetScaledResourcesPakFilePath(
    ui::ResourceScaleFactor scale_factor) {
  DCHECK(scale_factor == ui::k100Percent || scale_factor == ui::k200Percent);

  const char* pak_file = (scale_factor == ui::k100Percent)
                             ? "brave_100_percent.pak"
                             : "brave_200_percent.pak";
#if BUILDFLAG(IS_MAC)
  return base::apple::PathForFrameworkBundleResource(pak_file);
#else
  base::FilePath pak_path;
  base::PathService::Get(base::DIR_ASSETS, &pak_path);
  pak_path = pak_path.AppendASCII(pak_file);
  return pak_path;
#endif  // OS_MAC
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace

namespace brave {

void InitializeResourceBundle() {
#if BUILDFLAG(IS_ANDROID)
  ui::BraveLoadMainAndroidPackFile("assets/brave_resources.pak",
                                   base::FilePath());
  // brave_100_percent.pak is excluded now from the Android build because
  // its resources are not used
#else
  auto& rb = ui::ResourceBundle::GetSharedInstance();
  rb.AddDataPackFromPath(GetResourcesPakFilePath(), ui::kScaleFactorNone);
  rb.AddDataPackFromPath(GetScaledResourcesPakFilePath(ui::k100Percent),
                         ui::k100Percent);
  if (ui::IsScaleFactorSupported(ui::k200Percent)) {
    rb.AddDataPackFromPath(GetScaledResourcesPakFilePath(ui::k200Percent),
                           ui::k200Percent);
  }
#endif  // OS_ANDROID
}

// Returns true if this subprocess type needs the ResourceBundle initialized
// and resources loaded.
bool SubprocessNeedsResourceBundle() {
  auto cmd = *base::CommandLine::ForCurrentProcess();
#if BUILDFLAG(IS_IOS)
  return false;
#else
  std::string process_type = cmd.GetSwitchValueASCII(switches::kProcessType);
  return
#if BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_MAC)
      // The zygote process opens the resources for the renderers.
      process_type == switches::kZygoteProcess ||
#endif  // BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_MAC)
#if BUILDFLAG(IS_MAC)
      // Mac needs them too for scrollbar related images and for sandbox
      // profiles.
      process_type == switches::kPpapiPluginProcess ||
      process_type == switches::kGpuProcess ||
#endif  // BUILDFLAG(IS_MAC)
      process_type == switches::kRendererProcess ||
      process_type == switches::kUtilityProcess;
#endif  // BUILDFLAG(IS_IOS)
}

}  // namespace brave
