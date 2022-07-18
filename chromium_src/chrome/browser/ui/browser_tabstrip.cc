/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/buildflags/buildflags.h"
#if BUILDFLAG(ENABLE_INSTANT_NEW_TAB)
#include "brave/browser/new_tab/brave_new_tab_service.h"
#include "brave/browser/new_tab/brave_new_tab_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"
#endif

#if BUILDFLAG(ENABLE_INSTANT_NEW_TAB)
// Handles new tab creation logic. In case of multiple new tabs opened
// Inserts it before active opened new tab to avoid refreshing the page.
// Otherwise takes a new tab instance from cache and shows it instantly.
int OpenBraveNewTab(Browser* browser, NavigateParams* params, int idx) {
  auto* tab_strip_model = browser->tab_strip_model();
  auto* active_contents = tab_strip_model->GetActiveWebContents();
  int active_index = tab_strip_model->GetIndexOfWebContents(active_contents);
  if (active_index == tab_strip_model->GetTabCount() - 1 && active_contents) {
    auto is_new_tab_opened =
        active_contents->GetVisibleURL() == browser->GetNewTabURL();
    if (is_new_tab_opened) {
      params->disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
      // Prepend current tab by newly opened new tab
      return active_index;
    }
  }
  // Otherwise use cached instance.
  auto* new_tab_service =
      BraveNewTabServiceFactory::GetInstance()->GetServiceForContext(
          browser->profile());
  params->contents_to_insert = new_tab_service->GetNewTabContent();
  return idx;
}

#define tabstrip_index                            \
  tabstrip_index;                                 \
  if (params.url == browser->GetNewTabURL()) {    \
    idx = OpenBraveNewTab(browser, &params, idx); \
  }                                               \
  params.tabstrip_index

#endif
#include "src/chrome/browser/ui/browser_tabstrip.cc"
#if BUILDFLAG(ENABLE_INSTANT_NEW_TAB)
#undef tabstrip_index
#endif
