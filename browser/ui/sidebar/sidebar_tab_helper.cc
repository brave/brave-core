/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_tab_helper.h"

#include "base/metrics/field_trial_params.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/sidebar/features.h"
#include "brave/components/sidebar/pref_names.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace sidebar {

bool IsLeoPanelAlreadyOpened(content::WebContents* contents) {
  auto* prefs = user_prefs::UserPrefs::Get(contents->GetBrowserContext());
  return prefs->GetBoolean(kLeoPanelOneTimeOpen);
}

// static
void SidebarTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (!sidebar::features::kOpenOneShotLeoPanel.Get()) {
    return;
  }

  // For now, we only support leo panel for regular profile.
  auto* context = contents->GetBrowserContext();
  if (!brave::IsRegularProfile(context)) {
    return;
  }

  if (IsLeoPanelAlreadyOpened(contents)) {
    return;
  }

  content::WebContentsUserData<SidebarTabHelper>::CreateForWebContents(
      contents);
}

SidebarTabHelper::SidebarTabHelper(content::WebContents* contents)
    : WebContentsUserData(*contents) {
  Observe(contents);
}

SidebarTabHelper::~SidebarTabHelper() = default;

void SidebarTabHelper::PrimaryPageChanged(content::Page& page) {
  // It could be opened from other tabs after this helper is created.
  if (IsLeoPanelAlreadyOpened(web_contents())) {
    return;
  }

  const auto url = web_contents()->GetLastCommittedURL();
  if (!url.is_valid()) {
    return;
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  Browser* browser = chrome::FindBrowserWithTab(web_contents());
  if (!browser) {
    return;
  }

  auto* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  if (!service || !service->loaded()) {
    return;
  }

  auto* side_panel_ui = SidePanelUI::GetSidePanelUIForBrowser(browser);
  if (!side_panel_ui) {
    return;
  }

  // If side panel is already opened, don't open leo panel now.
  if (side_panel_ui->GetCurrentEntryId()) {
    return;
  }

  // Check this page to open one-time leo panel only for non-SERP page.
  for (TemplateURL* turl : service->GetTemplateURLs()) {
    GURL search_url(turl->url());
    if (!search_url.is_valid()) {
      continue;
    }

    // Don't launch leo panel for SERP page.
    if (search_url.host() == url.host()) {
      return;
    }
  }

  side_panel_ui->Show(SidePanelEntryId::kChatUI);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SidebarTabHelper);

}  // namespace sidebar
