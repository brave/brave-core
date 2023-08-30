/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ENGAGEMENT_HISTORY_AWARE_SITE_ENGAGEMENT_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ENGAGEMENT_HISTORY_AWARE_SITE_ENGAGEMENT_SERVICE_H_

#define UpdateEngagementScores                                                \
  UnusedFunction() {}                                                         \
                                                                              \
 protected:                                                                   \
  void HandleMediaPlaying(content::WebContents* web_contents, bool is_hidden) \
      override;                                                               \
  void HandleNavigation(content::WebContents* web_contents,                   \
                        ui::PageTransition transition) override;              \
  void HandleUserInput(content::WebContents* web_contents,                    \
                       EngagementType type) override;                         \
                                                                              \
 private:                                                                     \
  void UpdateEngagementScores

#include "src/chrome/browser/engagement/history_aware_site_engagement_service.h"  // IWYU pragma: export

#undef UpdateEngagementScores

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ENGAGEMENT_HISTORY_AWARE_SITE_ENGAGEMENT_SERVICE_H_
