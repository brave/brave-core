/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/check_op.h"
#include "base/dcheck_is_on.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/setup/brand_behaviors.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/install_util.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace installer {

namespace {

bool NavigateToUrlWithHttps(const std::wstring& url);
void NavigateToUrlWithIExplore(const std::wstring& url);

// This code is copied from `DoPostUninstallOperations` implementation in
// `chrome/installer/setup/google_chrome_behaviors.cc` with the following
// changes:
//
// - `distribution_data` not appended as Brave does not record histograms.
// - `kBraveUninstallSurveyUrl` used instead of `kUninstallSurveyUrl`

constexpr std::wstring_view kBraveUninstallSurveyUrl(
    L"https://brave.com/uninstall-survey/?p=brave_uninstall_survey");

}  // namespace

// If |archive_type| is INCREMENTAL_ARCHIVE_TYPE and |install_status| does not
// indicate a successful update, "-full" is appended to Chrome's "ap" value in
// its ClientState key if it is not present, resulting in the full installer
// being returned from the next update check. If |archive_type| is
// FULL_ARCHIVE_TYPE or |install_status| indicates a successful update, "-full"
// is removed from the "ap" value. "-stage:*" values are
// unconditionally removed from the "ap" value.
// This function used to be upstream and had to be restored in Brave to support
// delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
void UpdateInstallStatus(installer::ArchiveType archive_type,
                         installer::InstallStatus install_status) {
  GoogleUpdateSettings::UpdateInstallStatus(
      install_static::IsSystemInstall(), archive_type,
      InstallUtil::GetInstallReturnCode(install_status));
}

void DoPostUninstallOperations(const base::Version& version,
                               const base::FilePath& local_data_path,
                               const std::wstring& distribution_data) {
  // Send the Chrome version and OS version as params to the form. It would be
  // nice to send the locale, too, but I don't see an easy way to get that in
  // the existing code. It's something we can add later, if needed. We depend
  // on installed_version.GetString() not having spaces or other characters that
  // need escaping: 0.2.13.4. Should that change, we will need to escape the
  // string before using it in a URL.
  const base::win::OSInfo* os_info = base::win::OSInfo::GetInstance();
  base::win::OSInfo::VersionNumber version_number = os_info->version_number();
  const std::wstring os_version = base::ASCIIToWide(
      absl::StrFormat("%d.%d.%d", version_number.major, version_number.minor,
                      version_number.build));

  const std::wstring survey_url = std::wstring(kBraveUninstallSurveyUrl);
#if DCHECK_IS_ON()
  // The URL is expected to have a query part and not end with '&'.
  const size_t pos = survey_url.find(L'?');
  DCHECK_NE(pos, std::wstring::npos);
  DCHECK_EQ(survey_url.find(L'?', pos + 1), std::wstring::npos);
  DCHECK_NE(survey_url.back(), L'&');
#endif
  auto url = base::StrCat({survey_url, L"&crversion=",
                           base::ASCIIToWide(version.GetString()), L"&os=",
                           os_version});
  if (os_info->version() < base::win::Version::WIN10 ||
      !NavigateToUrlWithHttps(url)) {
    NavigateToUrlWithIExplore(url);
  }
}

class GoogleUpdateSettings_UNUSED {
 public:
  static void UpdateInstallStatus() { NOTREACHED(); }
};

}  // namespace installer

#define GoogleUpdateSettings GoogleUpdateSettings_UNUSED
#define DoPostUninstallOperations DoPostUninstallOperations_UNUSED
#include <chrome/installer/setup/google_chrome_behaviors.cc>
#undef DoPostUninstallOperations
#undef GoogleUpdateSettings
