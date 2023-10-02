/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/installer/setup/brand_behaviors.h"

#include <string_view>

#include "base/strings/stringprintf.h"

#define DoPostUninstallOperations DoPostUninstallOperations_UNUSED
#include "src/chrome/installer/setup/google_chrome_behaviors.cc"
#undef DoPostUninstallOperations

namespace installer {

namespace {

// This code is copied from `DoPostUninstallOperations` implementation in
// `chrome/installer/setup/google_chrome_behaviors.cc` with the following
// changes:
//
// - `distribution_data` not appended as Brave does not record histograms.
// - `kBraveUninstallSurveyUrl` used instead of `kUninstallSurveyUrl`

constexpr std::wstring_view kBraveUninstallSurveyUrl(
    L"https://brave.com/uninstall-survey/?p=brave_uninstall_survey");

}  // namespace

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
  std::wstring os_version =
      base::StringPrintf(L"%d.%d.%d", version_number.major,
                         version_number.minor, version_number.build);

  const std::wstring survey_url = std::wstring(kBraveUninstallSurveyUrl);
#if DCHECK_IS_ON()
  // The URL is expected to have a query part and not end with '&'.
  const size_t pos = survey_url.find(L'?');
  DCHECK_NE(pos, std::wstring::npos);
  DCHECK_EQ(survey_url.find(L'?', pos + 1), std::wstring::npos);
  DCHECK_NE(survey_url.back(), L'&');
#endif
  auto url = base::StringPrintf(L"%ls&crversion=%ls&os=%ls", survey_url.c_str(),
                                base::ASCIIToWide(version.GetString()).c_str(),
                                os_version.c_str());
  if (os_info->version() < base::win::Version::WIN10 ||
      !NavigateToUrlWithEdge(url)) {
    NavigateToUrlWithIExplore(url);
  }
}

}  // namespace installer
