/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/new_tab_html_source.h"

#include <string>
#include "brave/components/brave_shields/browser/brave_shields_stats.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"

using brave_shields::BraveShieldsStats;

void CustomizeNewTabHTMLSource(content::WebUIDataSource* source) {
  source->AddResourcePath("af7ae505a9eed503f8b8e6982036873e.woff2", IDR_BRAVE_COMMON_FONT_AWESOME_1);
  source->AddResourcePath("fee66e712a8a08eef5805a46892932ad.woff", IDR_BRAVE_COMMON_FONT_AWESOME_2);
  source->AddResourcePath("b06871f281fee6b241d60582ae9369b9.ttf", IDR_BRAVE_COMMON_FONT_AWESOME_3);
  source->AddResourcePath("img/toolbar/menu_btn.svg", IDR_BRAVE_COMMON_TOOLBAR_IMG);

  source->AddLocalizedString("adsBlocked", IDS_BRAVE_NEW_TAB_TOTAL_ADS_BLOCKED);
  source->AddLocalizedString("trackersBlocked", IDS_BRAVE_NEW_TAB_TOTAL_TRACKERS_BLOCKED);
  source->AddLocalizedString("httpsUpgraded", IDS_BRAVE_NEW_TAB_TOTAL_HTTPS_UPGRADES);
  source->AddLocalizedString("estimatedTimeSaved", IDS_BRAVE_NEW_TAB_TOTAL_TIME_SAVED);
  source->AddLocalizedString("thumbRemoved", IDS_BRAVE_NEW_TAB_THUMB_REMOVED);
  source->AddLocalizedString("undoRemoved", IDS_BRAVE_NEW_TAB_UNDO_REMOVED);
  source->AddLocalizedString("restoreAll", IDS_BRAVE_NEW_TAB_RESTORE_ALL);

  // Hash path is the MD5 of the file contents, webpack image loader does this
  source->AddResourcePath("fd85070af5114d6ac462c466e78448e4.svg", IDR_BRAVE_NEW_TAB_IMG1);
  source->AddResourcePath("314e7529efec41c8867019815f4d8dad.svg", IDR_BRAVE_NEW_TAB_IMG4);
  source->AddResourcePath("6c337c63662ee0ba4e57f6f8156d69ce.svg", IDR_BRAVE_NEW_TAB_IMG2);
  source->AddResourcePath("50cc52a4f1743ea74a21da996fe44272.jpg", IDR_BRAVE_NEW_TAB_IMG14);

  // Shield stats
  source->AddString("adsBlockedStat",
      std::to_string(BraveShieldsStats::GetInstance()->GetAdsBlocked()));
  source->AddString("trackersBlockedStat",
      std::to_string(BraveShieldsStats::GetInstance()->GetTrackersBlocked()));
  source->AddString("javascriptBlockedStat",
      std::to_string(BraveShieldsStats::GetInstance()->GetJavascriptBlocked()));
  source->AddString("httpsUpgradesStat",
      std::to_string(BraveShieldsStats::GetInstance()->GetHTTPSUpgrades()));
  source->AddString("fingerprintingBlockedStat",
      std::to_string(BraveShieldsStats::GetInstance()->GetFingerprintingBlocked()));

  source->SetJsonPath("strings.js");
}

