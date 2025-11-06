/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_VIEW_IDS_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_VIEW_IDS_H_

// View IDs for Brave-specific page info bubble views. These IDs are used to
// identify views for testing purposes. We start at a high number to avoid
// conflicts with Chromium's page info view IDs.
enum class BravePageInfoViewID {
  kTabSwitcherShieldsButton = 10000,
  kTabSwitcherSiteSettingsButton,
  kShieldsWebView
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_VIEW_IDS_H_
