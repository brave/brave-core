/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_about_handler.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/version_info.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/webui/settings/about_handler.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

const char kBraveBuildInstructionsUrl[] =
    "https://github.com/brave/brave-browser/wiki";
const char kBraveLicenseUrl[] = "https://mozilla.org/MPL/2.0/";
const char kBraveReleaseTagPrefix[] =
    "https://github.com/brave/brave-browser/releases/tag/v";

}  // namespace

namespace settings {

AboutHandler* BraveAboutHandler::Create(content::WebUIDataSource* html_source,
                                        Profile* profile) {
  AboutHandler* handler = AboutHandler::Create(html_source, profile);
  base::string16 license = l10n_util::GetStringFUTF16(
      IDS_BRAVE_VERSION_UI_LICENSE, base::ASCIIToUTF16(kBraveLicenseUrl),
      base::ASCIIToUTF16(chrome::kChromeUICreditsURL),
      base::ASCIIToUTF16(kBraveBuildInstructionsUrl),
      base::ASCIIToUTF16(kBraveReleaseTagPrefix) +
          base::UTF8ToUTF16(
              version_info::GetBraveVersionWithoutChromiumMajorVersion()));
  html_source->AddString("aboutProductLicense", license);
  return handler;
}

}  // namespace settings
