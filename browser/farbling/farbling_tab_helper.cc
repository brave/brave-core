/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/farbling/farbling_tab_helper.h"

#include <algorithm>
#include <string>

#include "base/system/sys_info.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_client.h"
#include "content/public/common/user_agent.h"
#include "net/http/http_util.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"

using brave_shields::ControlType;
using brave_shields::GetBraveShieldsEnabled;
using brave_shields::GetFingerprintingControlType;

namespace {

#if defined(OS_MAC)
int32_t GetMinimumBugfixVersion(int32_t os_major_version,
                                int32_t os_minor_version) {
  if (os_major_version == 10) {
    switch (os_minor_version) {
      case 9:
      case 10:
        return 5;
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        return 6;
    }
  }
  return 0;
}
#endif

std::string GetUserAgentPlatform() {
#if defined(OS_WIN)
  return "";
#elif defined(OS_MAC)
  return "Macintosh; ";
#elif defined(USE_X11)
  return "X11; ";
#elif defined(OS_ANDROID)
  return "Linux; ";
#elif defined(OS_POSIX)
  return "Unknown; ";
#endif
}

std::string GetMinimalProduct() {
  return version_info::GetProductNameAndVersionForUserAgent();
}

std::string GetMinimalOSVersion() {
  std::string os_version;
#if defined(OS_WIN) || defined(OS_MAC)
  int32_t os_major_version = 0;
  int32_t os_minor_version = 0;
  int32_t os_bugfix_version = 0;
  base::SysInfo::OperatingSystemVersionNumbers(
      &os_major_version, &os_minor_version, &os_bugfix_version);
#endif
#if defined(OS_MAC)
  os_bugfix_version =
      std::max(os_bugfix_version,
               GetMinimumBugfixVersion(os_major_version, os_minor_version));
#endif

#if defined(OS_ANDROID)
  std::string android_version_str = base::SysInfo::OperatingSystemVersion();
  std::string android_info_str =
      GetAndroidOSInfo(content::IncludeAndroidBuildNumber::Exclude,
                       content::IncludeAndroidModel::Exclude);
#endif

  base::StringAppendF(&os_version,
#if defined(OS_WIN)
                      "%d.%d", os_major_version, os_minor_version
#elif defined(OS_MAC)
                      "%d_%d_%d", os_major_version, os_minor_version,
                      os_bugfix_version
#elif defined(OS_ANDROID)
                      "%s%s", android_version_str.c_str(),
                      android_info_str.c_str()
#else
                      ""
#endif
  );  // NOLINT
  return os_version;
}

}  // namespace

namespace brave {

FarblingTabHelper::~FarblingTabHelper() = default;

FarblingTabHelper::FarblingTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void FarblingTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  UpdateUserAgent(navigation_handle);
}

void FarblingTabHelper::UpdateUserAgent(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle)
    return;
  content::WebContents* web_contents = navigation_handle->GetWebContents();
  if (!web_contents)
    return;
  std::string ua = "";
  Profile* profile = static_cast<Profile*>(web_contents->GetBrowserContext());
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  // If shields is off or farbling is off, do not override.
  // Also, we construct real user agent two different ways, through the browser
  // client's higher level utility function and through direct functions. If
  // they differ, there's some sort of override happening. Maybe the end
  // user is forcing the user agent via command line flags. Or maybe they
  // turned on the "freeze user agent" flag. Whatever it is, we want to
  // respect it.
  if (GetBraveShieldsEnabled(map, navigation_handle->GetURL()) &&
      (GetFingerprintingControlType(map, navigation_handle->GetURL()) !=
       ControlType::ALLOW) &&
      (GetUserAgent() ==
       content::BuildUserAgentFromProduct(
           version_info::GetProductNameAndVersionForUserAgent()))) {
    std::string minimal_os_info;
    base::StringAppendF(&minimal_os_info, "%s%s",
                        GetUserAgentPlatform().c_str(),
                        content::BuildOSCpuInfoFromOSVersionAndCpuType(
                            GetMinimalOSVersion(), content::BuildCpuInfo())
                            .c_str());
    ua = content::BuildUserAgentFromOSAndProduct(minimal_os_info,
                                                 GetMinimalProduct());
    navigation_handle->SetIsOverridingUserAgent(true);
    web_contents->SetUserAgentOverride(
        blink::UserAgentOverride::UserAgentOnly(ua),
        false /* override_in_new_tabs */);
  } else {
    navigation_handle->SetIsOverridingUserAgent(false);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(FarblingTabHelper)

}  // namespace brave
